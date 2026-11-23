#pragma once
class Bullet;
class Player;
class Sound;

#include <SDL3/SDL_audio.h>
#include <SDL.h>

class Controls {
private:
	
public:
	static void ShootBullet(Bullet*, Player*, Sound*, 
		SDL_AudioStream* audioPlayer);
};