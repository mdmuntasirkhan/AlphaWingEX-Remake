#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include "Sound.h"
#include "GameConstants.h"
#include <vector>
#include <string>

class SoundManager {
private:
	std::vector<SDL_AudioStream*> SFXStreamList;
	SDL_AudioStream*				  BGMStream; // for background music
	SDL_AudioDeviceID				 mainDevice; // the 12 SFX pipes share this device for mixing
	SDL_AudioDeviceID				 bgmDevice;  // BGMStream gets its own device, so pausing music never pauses SFX
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

	// Default stream format — stereo 16-bit, matches the project's WAV assets
	SDL_AudioSpec defaultSpec{ // default WAV format spec
		SDL_AUDIO_S16, // format
		2, // channel
		GameConst::kAudioSampleRate // frequency
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

	// Raw stream access — lets callers that manage their own Sound::Play()/gain/pause calls
	// (e.g. SceneMuntasir) borrow SoundManager-owned streams instead of creating their own.
	SDL_AudioStream* GetBGMStream() const { return BGMStream; }
	SDL_AudioStream* GetSFXPipe(int index) const { return SFXStreamList[index]; }
};

#endif // SOUNDMANAGER_H
