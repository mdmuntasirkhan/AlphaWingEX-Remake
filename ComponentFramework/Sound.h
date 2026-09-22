#ifndef SOUND_H
#define SOUND_H

#include <SDL3/SDL_audio.h>
#include <SDL.h>
#include <Vector.h>

class Sound {
private:
	SDL_AudioSpec   spec;
	Uint8*			soundBuffer;
	Uint32			soundLength;
	const char*		filename;

	void loadSound(const char* filename_);

public:
	Sound(const char* filename_);
	~Sound();

	bool OnCreate();
	void OnDestroy();
	bool   IsLoaded()   const { return soundBuffer != nullptr; }
	Uint32 GetLength()  const { return soundLength; }
	void Play(SDL_AudioStream* audioPlayer) const;
};

#endif // SOUND_H
