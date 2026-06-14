#pragma once
#include "Sound.h"
#include <vector>
#include <string>

class SoundManager {
public:
	SoundManager();
	~SoundManager();

	bool OnCreate();
	void OnDestroy();

	void playSoundAt(const Sound* sound, const int pipe);
	void playSoundAt(const Sound* sound); // automaticlly select a pipe that no sound is played

	void setBackgroundMusic(const char* filename);

	// volume adjustment
	// 0.0f - 1.0f
	// 0.0f = 0%
	// 1.0f = 100%
	// 0.5f = 50%
	void adjustBackgroundMusicVolume(const float value);
	void adjustMasterVolume(const float value);
	void adjustSFXVolume(const float value);

	void switchBackgroundMusic(const char* filename);

	void UpdateBGM(); // use on scene update

private:
	std::vector<SDL_AudioStream*> SFXStreamList;
	SDL_AudioStream* BGMStream; // for background music
	SDL_AudioDeviceID mainDevice;

	enum class SOUND_PIPE {
		PIPE1 = 0,
		PIPE2,
		PIPE3,
		PIPE4,
		PIPE5,
		PIPE6,
		PIPE7,
		PIPE8,
		PIPE9,
		PIPE10,
		PIPE11,
		PIPE12,
		MAX_NUMBER_PIPES
	};

	SDL_AudioSpec defaultSpec{ // default WAV format spec
		SDL_AUDIO_S16, // format
		2, // channel
		48000 // frequency
	};

	Sound* BGM;
};