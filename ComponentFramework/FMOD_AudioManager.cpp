#include "FMOD_AudioManager.h"
#include <fmod_errors.h>
#include <iostream>
#include <algorithm>

bool FMOD_AudioManager::OnCreate() {
	FMOD::Studio::System* rawSystem = nullptr; // need a raw pointer to initialize studio system
	FMOD_RESULT systemCheck = FMOD::Studio::System::create(&(rawSystem));

	if (systemCheck != FMOD_OK) { // check error
		std::cerr << "Initialization audio system Fail: " << FMOD_ErrorString(systemCheck) << std::endl;
		return false;
	}

	studioAudioSystem.reset(rawSystem); // transfer ownership

	systemCheck = studioAudioSystem->initialize(
		128, // channel
		FMOD_STUDIO_INIT_NORMAL, // Flag for studio system
		FMOD_INIT_NORMAL, // Flag for core system
		nullptr // extra driver data
	);

	if (systemCheck != FMOD_OK) {
		std::cerr << "Initialization audio system Fail: " << FMOD_ErrorString(systemCheck) << std::endl;
		OnDestroy();
		return false;
	}

	// Get the core system
	systemCheck = studioAudioSystem->getCoreSystem(&audioSystem);
	if (systemCheck != FMOD_OK) {
		std::cerr << "Initialization audio system Fail: " << FMOD_ErrorString(systemCheck) << std::endl;
		OnDestroy();
		return false;
	}

	// initializing the channels
	FMOD::ChannelGroup* rawBGMchannelGroup = nullptr;
	audioSystem->createChannelGroup("BGM", &rawBGMchannelGroup);

	masterChannelGroup.reset(rawBGMchannelGroup);

	return true;
}

void FMOD_AudioManager::OnDestroy() {
	// clear system and channel group
	if (masterChannelGroup) {
		masterChannelGroup->stop();
	}
	BGMchannel = nullptr;

	// Clear map, order does matter and event should be 
	eventMap.clear();
	bankMap.clear();
	audioMap.clear();

	masterChannelGroup.reset();

	audioSystem = nullptr;
	studioAudioSystem.reset();
}

void FMOD_AudioManager::Update() {
	if (!studioAudioSystem) {
		return;
	}

	// only need to update the studio audio system since the core audio system is included in it
	FMOD_RESULT systemCheck = studioAudioSystem->update();
	if (systemCheck != FMOD_OK) {
		std::cerr << "FMOD Runtime Err: " << FMOD_ErrorString(systemCheck) << std::endl;
	}
}

bool FMOD_AudioManager::loadAudio(const std::string& name, const std::string& path, bool loop = false) {
	// if Both system is not initialized
	if (!studioAudioSystem || !audioSystem || audioMap.find(name) != audioMap.end()) return false;

	FMOD_MODE mode = loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF; // Is the sound looping or not

	FMOD::Sound* rawSound = nullptr;
	FMOD_RESULT systemCheck = audioSystem->createSound(
		path.c_str(), // path of the File FMOD required a c-string
		mode, // loop mode
		nullptr, // optional loading setting
		&rawSound // where the sound pointer saves
	);

	if (systemCheck != FMOD_OK) {
		std::cerr << "FMOD Load Audio Err: " << FMOD_ErrorString(systemCheck) << std::endl;
		return false;
	}

	// ADD audio into map
	audioMap.emplace(name, FMODAudioPtr(rawSound));

	return true;
}

// Low Level audio system implementation -------- Relate to the core FMOD audio system
bool FMOD_AudioManager::PlaySFX(const std::string& name) {
	auto audio = audioMap.find(name);
	if (!studioAudioSystem || !audioSystem) return false;

	if (audio == audioMap.end()) {
		std::cout << "No such audio: " << name << std::endl;
		return false;
	}

	// each time the SFX plays it create a new instance
	audioSystem->playSound(
		audio->second.get(), // getting the Audio pointer
		nullptr, // Channel group pointer
		false, // is paused
		nullptr // Channel pointer
	);

	return true;
}

bool FMOD_AudioManager::PlayBGM(const std::string& name) {
	auto audio = audioMap.find(name);
	if (!studioAudioSystem || !audioSystem) return false;

	if (audio == audioMap.end()) {
		std::cout << "No such audio: " << name << std::endl;
		return false;
	}

	StopBGM();

	audioSystem->playSound(
		audio->second.get(), // getting the Audio pointer
		nullptr, // Channel group pointer
		false, // is paused
		&BGMchannel // Channel pointer
	);

	return true;
}

bool FMOD_AudioManager::StopBGM() {
	if (BGMchannel) {
		BGMchannel->stop();
		BGMchannel = nullptr;
		return true;
	}
	else {
		return false;
	}
}

bool FMOD_AudioManager::stopBGMEvent() {
	if (BGMInstance) {
		BGMInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
		BGMInstance.reset();
		return true;
	}
	else {
		return false;
	}
}

bool FMOD_AudioManager::SetBGMVolume(float volume) {
	if (BGMchannel) {
		// MIN: 0.0f -------- MAX: 1.0f
		volume = std::clamp(volume, 0.0f, 1.0f); // clamp the volume from 0.0f to 1.0f
		BGMchannel->setVolume(volume);
		return true;
	}
	else {
		return false;
	}
}

// High Level audio system implementation -------- Relate to the FMOD studio 

bool FMOD_AudioManager::loadBank(const std::string& name, const std::string& path)
{
	// if Both system is not initialized
	if (!studioAudioSystem || !audioSystem || bankMap.find(name) != bankMap.end()) return false;

	FMOD::Studio::Bank* rawBank = nullptr;

	FMOD_RESULT systemCheck = studioAudioSystem->loadBankFile(
		path.c_str(), // path of the File FMOD required a c-string
		FMOD_STUDIO_LOAD_BANK_NORMAL, // Flag for loading Bank
		&rawBank
	);

	if (systemCheck != FMOD_OK) {
		std::cerr << "FMOD Load Audio Err: " << FMOD_ErrorString(systemCheck) << std::endl;
		return false;
	}

	// ADD audio into map
	bankMap.emplace(name, FMODBankPtr(rawBank));

	return true;
}

bool FMOD_AudioManager::loadEvent(const std::string& name, const std::string& path) {
	// if Both system is not initialized
	if (!studioAudioSystem || !audioSystem || eventMap.find(name) != eventMap.end()) return false;

	FMOD::Studio::EventDescription* rawEventDescription = nullptr;

	FMOD_RESULT systemCheck = studioAudioSystem->getEvent(
		path.c_str(), // Should be assign to a loaded bank, Sample Path = "event:/Audio/sample"
		&rawEventDescription
	);

	if (systemCheck != FMOD_OK) {
		std::cerr << "FMOD Load Event Err: " << FMOD_ErrorString(systemCheck) << std::endl;
		return false;
	}

	eventMap.emplace(name, rawEventDescription);

	return true;
}

// This function will play the instance once whenever this funcion calls
// The instance will not save and release after use
bool FMOD_AudioManager::PlaySFXEvent(const std::string& name) {
	if (!studioAudioSystem || !audioSystem ) return false;

	auto audioEvent = eventMap.find(name);

	if (audioEvent != eventMap.end()) {
		std::cout << "No such audio event: " << name << std::endl;
		return false;
	}

	FMOD::Studio::EventInstance* tempInstance = nullptr;

	FMOD_RESULT systemCheck = audioEvent->second->createInstance(&tempInstance);

	if (systemCheck != FMOD_OK) {
		std::cerr << "FMOD event instance creation Err: " << FMOD_ErrorString(systemCheck) << std::endl;
		return false;
	}

	// play the SFX
	systemCheck = tempInstance->start();

	if (systemCheck != FMOD_OK) {
		std::cerr << "FMOD event instance creation Err: " << FMOD_ErrorString(systemCheck) << std::endl;
		tempInstance->release();
		return false;
	}

	// release the instance at the end
	tempInstance->release();

	return true;
}

bool FMOD_AudioManager::PlayBGMEvent(const std::string& name) {
	auto audio = eventMap.find(name);
	if (!studioAudioSystem || !audioSystem) return false;

	if (audio == eventMap.end()) {
		std::cout << "No such audio: " << name << std::endl;
		return false;
	}

	// stops the music
	stopBGMEvent();

	FMOD::Studio::EventInstance* tempInstance;

	FMOD_RESULT systemCheck = audio->second->createInstance(&tempInstance);

	if (systemCheck != FMOD_OK) {
		std::cerr << "FMOD BGM event Err: " << FMOD_ErrorString(systemCheck) << std::endl;
		tempInstance->release();
		return false;
	}

	BGMInstance.reset(tempInstance);

	BGMInstance->start();

	return true;
}

bool FMOD_AudioManager::setBGMintiensive(const float intensive) {
	if (!BGMInstance) return false;

	// using magic number just for now
	FMOD_RESULT systemCheck = BGMInstance->setParameterByName("intensive", std::clamp(intensive, 0.0f, 5.0f));

	if (systemCheck != FMOD_OK) {
		std::cerr << "FMOD parameter Err: " << FMOD_ErrorString(systemCheck) << std::endl;
		return false;
	}

	return true;
}