#pragma once

#include <fmod.hpp>
#include <fmod_studio.hpp>
#include <memory>
#include <unordered_map>
#include <string>

// Custom deleter
// use for releasing memory from FMOD system
// since smart point won't automatically release the memory allocated from FMOD studio, we need to create our own
struct StudioAudioSystemDltr {
	void operator()(FMOD::Studio::System* system) const noexcept {
		if (system) {
			system->release();
		}
	}
};

struct AudioSoundDltr {
	void operator()(FMOD::Sound* sound) const noexcept {
		if (sound) {
			sound->release();
		}
	}
};

struct BankSoundDltr {
	void operator()(FMOD::Studio::Bank* bank) const noexcept { 
		if (bank) {
			bank->unload();
		}
	}
};

struct FmodEventInstanceDltr {
	void operator()(FMOD::Studio::EventInstance* event) const noexcept {
		if (event) {
			event->stop(FMOD_STUDIO_STOP_IMMEDIATE);
			event->release();
		}
	}
};

struct FmodChannelGroupDltr {
	void operator()(FMOD::ChannelGroup* channelGroup) {
		if (channelGroup) {
			channelGroup->release();
		}
	}
};

// convinent setup
using FMODAudioPtr = std::unique_ptr<FMOD::Sound, AudioSoundDltr>;
using FMODBankPtr = std::unique_ptr<FMOD::Studio::Bank, BankSoundDltr>;
using FMODInstancePtr = std::unique_ptr<FMOD::Studio::EventInstance, FmodEventInstanceDltr>;

class FMOD_AudioManager {
public:
	// avoid copying it self
	FMOD_AudioManager(const FMOD_AudioManager&) = delete;
	FMOD_AudioManager& operator=(const FMOD_AudioManager&) = delete;

	FMOD_AudioManager() = default;
	~FMOD_AudioManager() = default;

	bool OnCreate();
	void Update();
	void OnDestroy();

	bool loadAudio(const std::string& name, const std::string& path, bool loop);
	bool loadBank(const std::string& name, const std::string& path);
	bool loadEvent(const std::string& name, const std::string& path);
	
	bool PlaySFX(const std::string& name);
	bool PlayBGM(const std::string& name);
	bool StopBGM();
	bool SetBGMVolume(float volume);
	
private:
	/*
	*	audioSystem is the basic raw system for FMOD api
	*	studioAudioSystem is a higher level of audioSystem where it can take care of Fmod banks
	*/

	// use a Raw pointer because it will use the system from StudioAudioSystem
	// if after studioAudioSystem release it self audioSystem also get released
	FMOD::System* audioSystem = nullptr;
	std::unique_ptr<FMOD::Studio::System, StudioAudioSystemDltr> studioAudioSystem;
	
	/*
	* Map cover 3 different catory in FMOD
	* audioMap is the basic audio file that will play
	* bankMap will be the bank files created by FMOD Studio
	* eventMap will be the event instance that takes control of how the audio plays
	*/

	std::unordered_map<std::string, FMODAudioPtr> audioMap;
	std::unordered_map<std::string, FMODBankPtr> bankMap;
	std::unordered_map<std::string, FMOD::Studio::EventDescription* > eventMap; // Bank has the owner ship of event description

	// sample channel
	std::unique_ptr<FMOD::ChannelGroup, FmodChannelGroupDltr> masterChannelGroup;
	FMOD::Channel* BGMchannel = nullptr; // same reason for BGM channel since it's owner ship is under ChannelGroup

};