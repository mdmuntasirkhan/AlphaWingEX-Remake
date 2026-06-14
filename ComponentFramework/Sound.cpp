#include "Sound.h"

Sound::Sound(const char* filename_) : soundBuffer{nullptr}, soundLength{0} {
	filename = filename_;
}

Sound::~Sound() { /* free memory */ };

bool Sound::OnCreate() {
	loadSound(filename);
	return true;
}

void Sound::OnDestroy() {
	SDL_free(soundBuffer);
}

void Sound::loadSound(const char* filename_) {

	if (!SDL_LoadWAV(filename_, &spec, &soundBuffer, &soundLength)) {
		std::cout << ("Fail to load sound");
	}

	SDL_Log("Freq: %d", spec.freq);
	SDL_Log("Channels: %d", spec.channels);
	SDL_Log("Format: %u", spec.format);

}

void Sound::Play(SDL_AudioStream* audioPlayer) const {
	if (!SDL_PutAudioStreamData(audioPlayer, soundBuffer, soundLength))
	{
		std::cout << "Fail to play sound\n";
	}
}