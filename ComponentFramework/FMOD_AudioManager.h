#pragma once

#include <fmod.hpp>
#include <memory>
#include <unordered_map>
#include <string>

class FMOD_AudioManager {
public:
	// avoid copying it self
	FMOD_AudioManager(const FMOD_AudioManager&) = delete;
	FMOD_AudioManager& operator=(const FMOD_AudioManager&) = delete;

	FMOD_AudioManager();
	~FMOD_AudioManager();

	void PlaySFX();
	void PlayBGM();
	void StopBGM();
	void SetBGMVolume(float volume);

private:
	std::unique_ptr<FMOD::System> audioSystem = std::make_unique<FMOD::System>();
	std::unordered_map<std::string, std::unique_ptr<FMOD::Sound>> audioMap;
	std::unique_ptr<FMOD::Channel> BGMchannel = std::make_unique<FMOD::Channel>();

};