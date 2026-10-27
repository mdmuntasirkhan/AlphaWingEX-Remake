#include "Controls.h"
#include "Bullet.h"
#include "Player.h"
#include "Sound.h"

void Controls::ShootBullet(Bullet* bullet, Player* player, Sound* sfx, 
	SDL_AudioStream* audioPlayer)
{
	bullet->Spawn(player->GetPosition());
	player->ApplyImpulse(Vec3(-1.5f, 0.0f, 0.0f)); // recoil
	sfx->Play(audioPlayer);
}
