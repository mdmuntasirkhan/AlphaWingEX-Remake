#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include "Sound.h"
#include <vector>
#include <string>

class SoundManager {
private:
	std::vector<SDL_AudioStream*> SFXStreamList;
	SDL_AudioStream*				  BGMStream; // for background music
	SDL_AudioDeviceID				 mainDevice;
	Sound*									BGM;

	// Audio pipe pool — SFX is routed through whichever pipe is free
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

	// Default stream format — stereo 16-bit 48 kHz
	SDL_AudioSpec defaultSpec{ // default WAV format spec
		SDL_AUDIO_S16, // format
		2, // channel
		48000 // frequency
	};

public:
	SoundManager();
	~SoundManager();

	bool OnCreate();
	void OnDestroy();

	// Plays on a specific pipe
	void playSoundAt(const Sound* sound, const int pipe);
	void playSoundAt(const Sound* sound); // automaticlly select a pipe that no sound is played

	void setBackgroundMusic(const char* filename);
	void switchBackgroundMusic(const char* filename);
	void UpdateBGM(); // use on scene update

	// volume adjustment range 0.0 = silent, 1.0 = full
	void adjustBackgroundMusicVolume(const float value);
	void adjustMasterVolume(const float value) const;
	void adjustSFXVolume(const float value);
};

#endif // SOUNDMANAGER_H
