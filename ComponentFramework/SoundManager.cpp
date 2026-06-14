#include "SoundManager.h"

SoundManager::SoundManager() : BGM(nullptr), BGMStream(nullptr) {
	mainDevice = SDL_OpenAudioDevice(
		SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
		&defaultSpec
	);
}

SoundManager::~SoundManager() { /* Free Memory */ }

bool SoundManager::OnCreate() {
	for (size_t i = 0; i < (size_t)SOUND_PIPE::MAX_NUMBER_PIPES; i++) {
		SFXStreamList.push_back(SDL_CreateAudioStream(&defaultSpec, &defaultSpec));
		SDL_BindAudioStream(mainDevice, SFXStreamList[i]);
	}

	// background music
	BGMStream = SDL_CreateAudioStream(&defaultSpec, &defaultSpec);
	SDL_BindAudioStream(mainDevice, BGMStream);
	return true;
}

void SoundManager::OnDestroy() {
	for (SDL_AudioStream* audioPipe: SFXStreamList) {
		SDL_DestroyAudioStream(audioPipe);
	}

	if (BGMStream) {
		SDL_DestroyAudioStream(BGMStream);
	}

	if (BGM) {
		BGM->OnDestroy();
		delete BGM;
	}
}

// for sound effects
void SoundManager::playSoundAt(const Sound* sound, const int pipe) {
	if (pipe < (int)SOUND_PIPE::MAX_NUMBER_PIPES) {
		SDL_ClearAudioStream(SFXStreamList[pipe]);
		sound->Play(SFXStreamList[pipe]);
	}
	else {
		throw std::runtime_error("Exceed the maximum number of pipes");
	}
}

void SoundManager::playSoundAt(const Sound* sound) {
	bool foundPipe = false;
	for (SDL_AudioStream* audioPipe : SFXStreamList) {
		if (SDL_GetAudioStreamQueued(audioPipe) == 0) {
			sound->Play(audioPipe);
			foundPipe = true;
			break;
		}
	}

	if (!foundPipe) { // if all pipes are in queued, play the sound at the first pipe
		SDL_ClearAudioStream(SFXStreamList[0]);
		sound->Play(SFXStreamList[0]); 
	}
}

// background music specific

void SoundManager::setBackgroundMusic(const char* filename) {
	BGM = new Sound(filename);
	BGM->OnCreate();
}

void SoundManager::switchBackgroundMusic(const char* filename) {
	SDL_ClearAudioStream(BGMStream);
	BGM->OnDestroy(); // delete the original one

	BGM = new Sound(filename);
	BGM->OnCreate();
}

void SoundManager::UpdateBGM() {
	if (BGM) {
		BGM->Play(BGMStream);
	}
}

void SoundManager::adjustBackgroundMusicVolume(const float value) {
	SDL_SetAudioStreamGain(BGMStream, value);
}

void SoundManager::adjustMasterVolume(const float value) {
	SDL_SetAudioDeviceGain(mainDevice, value);
}

void SoundManager::adjustSFXVolume(const float value) {
	for (SDL_AudioStream* audioPipe : SFXStreamList) {
		SDL_SetAudioStreamGain(audioPipe, value);
	}
}