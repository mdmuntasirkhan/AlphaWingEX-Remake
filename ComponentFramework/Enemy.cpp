#include "Enemy.h"
#include <cstdlib>
#include <cmath>

void Enemy::SpawnHitDebris(const Vec3& pos, const Vec3& color, int count) {
	for (int i = 0; i < count; i++) {
		Debris d;
		d.pos         = pos;
		d.color       = color;
		float angle   = (float)(rand() % 360) * 3.14159f / 180.0f;
		float spd     = 1.0f + (rand() % 20) * 0.05f;
		d.vel         = Vec3(cosf(angle) * spd, sinf(angle) * spd, 0.0f);
		d.angle       = (float)(rand() % 360);
		d.spinSpeed   = (float)((rand() % 361) - 180);
		d.maxLifetime = 0.3f + (rand() % 30) * 0.01f;
		d.lifetime    = d.maxLifetime;
		d.pieceScale  = 0.008f + (rand() % 3) * 0.004f;
		debris.push_back(d);
	}
}

void Enemy::SpawnKillDebris(const Vec3& pos, const Vec3& color, int count) {
	for (int i = 0; i < count; i++) {
		Debris d;
		d.pos         = pos;
		d.color       = color;
		float angle   = (float)(rand() % 360) * 3.14159f / 180.0f;
		float spd     = 5.0f + (rand() % 40) * 0.1f;
		d.vel         = Vec3(cosf(angle) * spd, sinf(angle) * spd, 0.0f);
		d.angle       = (float)(rand() % 360);
		d.spinSpeed   = (float)((rand() % 361) - 180);
		d.maxLifetime = 1.8f + (rand() % 70) * 0.01f;
		d.lifetime    = d.maxLifetime;
		d.pieceScale  = 0.025f + (rand() % 4) * 0.005f;
		debris.push_back(d);
	}
}
