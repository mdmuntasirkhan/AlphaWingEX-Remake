#include "Enemy.h"
#include <MMath.h>
#include <glew.h>
#include <iostream>
#include <cstdlib>
#include <cmath>

Enemy::Enemy() :
	asteroidMesh{ nullptr },
	bot01Mesh{ nullptr },
	bot01ThrustMesh{ nullptr },
	thrustTimer{ 0.0f },
	bot01SteerForce{ 3.0f },
	bot01YDamping{ 4.5f },
	bot01YMaxSpeed{ 2.5f },
	asteroidSpeed{ 0.9f },
	smallAsteroidSpeed{ 1.6f },
	bot01Speed{ 1.2f },
	asteroidSpawnTimer{ 0.0f },
	asteroidSpawnInterval{ 3.2f },
	smallAsteroidSpawnTimer{ 0.0f },
	smallAsteroidSpawnInterval{ 2.0f },
	bot01SpawnTimer{ 0.0f },
	bot01SpawnInterval{ 4.5f },
	totalTime{ 0.0f }
{
	// Leave Empty
}

Enemy::~Enemy() {}

bool Enemy::OnCreate(const char* asteroidFile,
					 const char* bot01File) {
	// Asteroid mesh
	asteroidMesh = new Mesh(asteroidFile);
	if (asteroidMesh->OnCreate() == false) {
		std::cout << " Asteroid mesh not found!\n";
		return false;
	}

	// Bot01 mesh
	bot01Mesh = new Mesh(bot01File);
	if (bot01Mesh->OnCreate() == false) {
		std::cout << "Bot01 mesh not found!\n";
		return false;
	}

	// Bot01 thrust mesh
	bot01ThrustMesh = new Mesh("meshes/Temp_AlphaWing_Enemy_Bot01_Thrust.obj");
	if (bot01ThrustMesh->OnCreate() == false) {
		std::cout << "Bot01 thrust mesh not found!\n";
		return false;
	}

	return true;

}

void Enemy::OnDestroy() {
	if (asteroidMesh) {
		asteroidMesh->OnDestroy();
		delete asteroidMesh;
		asteroidMesh = nullptr;
	}
	if (bot01Mesh) {
		bot01Mesh->OnDestroy();
		delete bot01Mesh;
		bot01Mesh = nullptr;
	}
	if (bot01ThrustMesh) {
		bot01ThrustMesh->OnDestroy();
		delete bot01ThrustMesh;
		bot01ThrustMesh = nullptr;
	}
	asteroidPositions.clear();
	asteroidAngles.clear();
	asteroidSpinSpeeds.clear();
	asteroidHP.clear();
	asteroidScales.clear();
	smallAsteroidPositions.clear();
	smallAsteroidAngles.clear();
	smallAsteroidSpinSpeeds.clear();
	smallAsteroidHP.clear();
	smallAsteroidScales.clear();
	bot01Positions.clear();
	bot01YVelocities.clear();
	bot01HP.clear();
	bot01HitTimers.clear();
	debris.clear();
}

void Enemy::Update(float deltaTime, float playerY) {
	totalTime += deltaTime;
	thrustTimer += deltaTime;

	// Large asteroids
	asteroidSpawnTimer += deltaTime;
	if (asteroidSpawnTimer >= asteroidSpawnInterval) {
		asteroidSpawnTimer = 0.0f;
		float randomY = ((rand() % 10) - 5) * 0.5f;
		asteroidPositions.push_back(Vec3(15.0f, randomY, -10.0f));
		asteroidAngles.push_back(0.0f);
		asteroidSpinSpeeds.push_back((float)((rand() % 121) - 60));
		asteroidHP.push_back(6);
		asteroidScales.push_back(0.4f);
	}
	for (int i = 0; i < (int)asteroidPositions.size(); i++) {
		asteroidPositions[i].x -= asteroidSpeed * deltaTime;
		asteroidAngles[i]      += asteroidSpinSpeeds[i] * deltaTime;
	}
	for (int i = (int)asteroidPositions.size() - 1; i >= 0; i--) {
		if (asteroidPositions[i].x < -15.0f) {
			asteroidPositions.erase(asteroidPositions.begin() + i);
			asteroidAngles.erase(asteroidAngles.begin() + i);
			asteroidSpinSpeeds.erase(asteroidSpinSpeeds.begin() + i);
			asteroidHP.erase(asteroidHP.begin() + i);
			asteroidScales.erase(asteroidScales.begin() + i);
		}
	}

	// Small asteroids
	smallAsteroidSpawnTimer += deltaTime;
	if (smallAsteroidSpawnTimer >= smallAsteroidSpawnInterval) {
		smallAsteroidSpawnTimer = 0.0f;
		float randomY = ((rand() % 14) - 7) * 0.5f;
		smallAsteroidPositions.push_back(Vec3(15.0f, randomY, -10.0f));
		smallAsteroidAngles.push_back(0.0f);
		smallAsteroidSpinSpeeds.push_back((float)((rand() % 181) - 90));
		smallAsteroidHP.push_back(3);
		smallAsteroidScales.push_back(0.2f);
	}
	for (int i = 0; i < (int)smallAsteroidPositions.size(); i++) {
		smallAsteroidPositions[i].x -= smallAsteroidSpeed * deltaTime;
		smallAsteroidAngles[i]      += smallAsteroidSpinSpeeds[i] * deltaTime;
	}
	for (int i = (int)smallAsteroidPositions.size() - 1; i >= 0; i--) {
		if (smallAsteroidPositions[i].x < -15.0f) {
			smallAsteroidPositions.erase(smallAsteroidPositions.begin() + i);
			smallAsteroidAngles.erase(smallAsteroidAngles.begin() + i);
			smallAsteroidSpinSpeeds.erase(smallAsteroidSpinSpeeds.begin() + i);
			smallAsteroidHP.erase(smallAsteroidHP.begin() + i);
			smallAsteroidScales.erase(smallAsteroidScales.begin() + i);
		}
	}

	// Bot01 (wave 2, after 30 s)
	if (totalTime > 30.0f) {
		bot01SpawnTimer += deltaTime;
		if (bot01SpawnTimer >= bot01SpawnInterval) {
			bot01SpawnTimer = 0.0f;
			float randomY = ((rand() % 10) - 5) * 0.5f;
			bot01Positions.push_back(Vec3(15.0f, randomY, -10.0f));
			bot01YVelocities.push_back(0.0f);
			bot01HP.push_back(10);
			bot01HitTimers.push_back(0.0f);
		}
	}
	for (int i = 0; i < (int)bot01Positions.size(); i++) {
		bot01Positions[i].x -= bot01Speed * deltaTime;

		// Spring-steer toward player Y
		float force = bot01SteerForce * (playerY - bot01Positions[i].y)
		            - bot01YDamping  * bot01YVelocities[i];
		bot01YVelocities[i] += force * deltaTime;
		if (bot01YVelocities[i] >  bot01YMaxSpeed) bot01YVelocities[i] =  bot01YMaxSpeed;
		if (bot01YVelocities[i] < -bot01YMaxSpeed) bot01YVelocities[i] = -bot01YMaxSpeed;
		bot01Positions[i].y += bot01YVelocities[i] * deltaTime;
	}
	for (int i = (int)bot01Positions.size() - 1; i >= 0; i--) {
		if (bot01Positions[i].x < -15.0f) {
			bot01Positions.erase  (bot01Positions.begin()   + i);
			bot01YVelocities.erase(bot01YVelocities.begin() + i);
			bot01HP.erase         (bot01HP.begin()          + i);
			bot01HitTimers.erase  (bot01HitTimers.begin()   + i);
		}
	}

	// Decrement bot01 hit flash timers
	for (int i = 0; i < (int)bot01HitTimers.size(); i++) {
		if (bot01HitTimers[i] > 0.0f) {
			bot01HitTimers[i] -= deltaTime;
			if (bot01HitTimers[i] < 0.0f) bot01HitTimers[i] = 0.0f;
		}
	}

	// Debris
	for (int i = (int)debris.size() - 1; i >= 0; i--) {
		debris[i].pos     += debris[i].vel * deltaTime;
		debris[i].angle   += debris[i].spinSpeed * deltaTime;
		debris[i].lifetime -= deltaTime;
		if (debris[i].lifetime <= 0.0f)
			debris.erase(debris.begin() + i);
	}
}

void Enemy::Render(Shader* shader,
	const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const {
	glUniformMatrix4fv(shader->GetUniformID("projectionMatrix"), 1, GL_FALSE, projectionMatrix);
	glUniformMatrix4fv(shader->GetUniformID("viewMatrix"), 1, GL_FALSE, viewMatrix);

	// Large asteroids — ORANGE, spinning, scale shrinks with damage
	glUniform4f(shader->GetUniformID("color"), 1.0f, 0.3f, 0.0f, 1.0f);
	for (int i = 0; i < (int)asteroidPositions.size(); i++) {
		float s = asteroidScales[i];
		Matrix4 m = MMath::translate(asteroidPositions[i]) *
		            MMath::rotate(asteroidAngles[i], Vec3(0.0f, 0.0f, 1.0f)) *
		            MMath::scale(s, s, s);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
		asteroidMesh->Render();
	}

	// Small asteroids — GREY, spinning, scale shrinks with damage
	glUniform4f(shader->GetUniformID("color"), 0.6f, 0.6f, 0.6f, 1.0f);
	for (int i = 0; i < (int)smallAsteroidPositions.size(); i++) {
		float s = smallAsteroidScales[i];
		Matrix4 m = MMath::translate(smallAsteroidPositions[i]) *
		            MMath::rotate(smallAsteroidAngles[i], Vec3(0.0f, 0.0f, 1.0f)) *
		            MMath::scale(s, s, s);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
		asteroidMesh->Render();
	}

	// Bot01 — RED, flashes white on hit
	for (int i = 0; i < (int)bot01Positions.size(); i++) {
		if (bot01HitTimers[i] > 0.0f)
			glUniform4f(shader->GetUniformID("color"), 1.0f, 1.0f, 1.0f, 1.0f);
		else
			glUniform4f(shader->GetUniformID("color"), 1.0f, 0.1f, 0.1f, 1.0f);
		Matrix4 m = MMath::translate(bot01Positions[i]) *
			MMath::rotate(180.0f, Vec3(0.0f, 1.0f, 0.0f)) *
			MMath::scale(0.17f, 0.17f, 0.17f);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
		bot01Mesh->Render();
	}

	// Bot01 thrust — additive blue electric pulse
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDepthMask(GL_FALSE);
	glUniform1f(shader->GetUniformID("emissive"), 1.0f);
	float pulse = 0.65f + 0.25f * sinf(thrustTimer * 20.0f)
	                    + 0.10f * sinf(thrustTimer * 37.0f);
	glUniform4f(shader->GetUniformID("color"), 0.0f, 0.4f * pulse, 1.0f, pulse);
	for (int i = 0; i < (int)bot01Positions.size(); i++) {
		Matrix4 m = MMath::translate(bot01Positions[i]) *
			MMath::rotate(180.0f, Vec3(0.0f, 1.0f, 0.0f)) *
			MMath::scale(0.17f, 0.17f, 0.17f);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
		bot01ThrustMesh->Render();
	}
	glUniform1f(shader->GetUniformID("emissive"), 0.0f);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	// Debris — spinning fragments, fade out over lifetime
	if (!debris.empty()) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glDepthMask(GL_FALSE);
		glUniform1f(shader->GetUniformID("emissive"), 1.0f);
		for (int i = 0; i < (int)debris.size(); i++) {
			float alpha = debris[i].lifetime / debris[i].maxLifetime;
			glUniform4f(shader->GetUniformID("color"),
				debris[i].color.x, debris[i].color.y, debris[i].color.z, alpha);
			float ps = debris[i].pieceScale;
			Matrix4 m = MMath::translate(debris[i].pos) *
			            MMath::rotate(debris[i].angle, Vec3(0.0f, 0.0f, 1.0f)) *
			            MMath::scale(ps, ps, ps);
			glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
			asteroidMesh->Render();
		}
		glUniform1f(shader->GetUniformID("emissive"), 0.0f);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}
}

// Tiny chips on each hit — slow, short-lived
void Enemy::SpawnHitDebris(const Vec3& pos, const Vec3& color, int count) {
	for (int i = 0; i < count; i++) {
		Debris d;
		d.pos         = pos;
		d.color       = color;
		float angle   = (float)(rand() % 360) * 3.14159f / 180.0f;
		float spd     = 1.0f + (rand() % 20) * 0.05f;          // 1.0..2.0
		d.vel         = Vec3(cosf(angle) * spd, sinf(angle) * spd, 0.0f);
		d.angle       = (float)(rand() % 360);
		d.spinSpeed   = (float)((rand() % 361) - 180);
		d.maxLifetime = 0.3f + (rand() % 30) * 0.01f;           // 0.3..0.6 s
		d.lifetime    = d.maxLifetime;
		d.pieceScale  = 0.008f + (rand() % 3) * 0.004f;         // 0.008..0.016
		debris.push_back(d);
	}
}

// Kill burst — fast, long-lived, sails off screen; always smaller than dying asteroid
void Enemy::SpawnKillDebris(const Vec3& pos, const Vec3& color, int count) {
	for (int i = 0; i < count; i++) {
		Debris d;
		d.pos         = pos;
		d.color       = color;
		float angle   = (float)(rand() % 360) * 3.14159f / 180.0f;
		float spd     = 5.0f + (rand() % 40) * 0.1f;           // 5..9 units/s
		d.vel         = Vec3(cosf(angle) * spd, sinf(angle) * spd, 0.0f);
		d.angle       = (float)(rand() % 360);
		d.spinSpeed   = (float)((rand() % 361) - 180);
		d.maxLifetime = 1.8f + (rand() % 70) * 0.01f;           // 1.8..2.5 s
		d.lifetime    = d.maxLifetime;
		d.pieceScale  = 0.025f + (rand() % 4) * 0.005f;         // 0.025..0.040
		debris.push_back(d);
	}
}

// Bullet/missile hit — chip away HP, shrink, spawn chunk, returns true if destroyed
bool Enemy::DamageAsteroid(int index) {
	asteroidHP[index]--;
	asteroidScales[index] *= 0.85f;
	SpawnHitDebris(asteroidPositions[index], Vec3(1.0f, 0.5f, 0.0f), 2);

	if (asteroidHP[index] <= 0) {
		SpawnKillDebris(asteroidPositions[index], Vec3(1.0f, 0.5f, 0.0f), 6);
		asteroidPositions.erase (asteroidPositions.begin()  + index);
		asteroidAngles.erase    (asteroidAngles.begin()     + index);
		asteroidSpinSpeeds.erase(asteroidSpinSpeeds.begin() + index);
		asteroidHP.erase        (asteroidHP.begin()         + index);
		asteroidScales.erase    (asteroidScales.begin()     + index);
		return true;
	}
	return false;
}

bool Enemy::DamageSmallAsteroid(int index) {
	smallAsteroidHP[index]--;
	smallAsteroidScales[index] *= 0.78f;
	SpawnHitDebris(smallAsteroidPositions[index], Vec3(0.6f, 0.6f, 0.6f), 1);

	if (smallAsteroidHP[index] <= 0) {
		SpawnKillDebris(smallAsteroidPositions[index], Vec3(0.6f, 0.6f, 0.6f), 4);
		smallAsteroidPositions.erase (smallAsteroidPositions.begin()  + index);
		smallAsteroidAngles.erase    (smallAsteroidAngles.begin()     + index);
		smallAsteroidSpinSpeeds.erase(smallAsteroidSpinSpeeds.begin() + index);
		smallAsteroidHP.erase        (smallAsteroidHP.begin()         + index);
		smallAsteroidScales.erase    (smallAsteroidScales.begin()     + index);
		return true;
	}
	return false;
}

// Bot01 takes a bullet/missile hit
bool Enemy::DamageBot01(int index) {
	bot01HP[index]--;
	if (bot01HP[index] <= 0) {
		SpawnKillDebris(bot01Positions[index], Vec3(1.0f, 0.2f, 0.2f), 6);
		bot01Positions.erase  (bot01Positions.begin()   + index);
		bot01YVelocities.erase(bot01YVelocities.begin() + index);
		bot01HP.erase         (bot01HP.begin()          + index);
		bot01HitTimers.erase  (bot01HitTimers.begin()   + index);
		return true;
	}
	// Non kill hit with white flash + tiny yellow sparks
	bot01HitTimers[index] = 0.12f;
	SpawnHitDebris(bot01Positions[index], Vec3(1.0f, 0.95f, 0.3f), 3);
	return false;
}

// Instant kill
void Enemy::RemoveAsteroid(int index) {
	SpawnKillDebris(asteroidPositions[index], Vec3(1.0f, 0.5f, 0.0f), 6);
	asteroidPositions.erase (asteroidPositions.begin()  + index);
	asteroidAngles.erase    (asteroidAngles.begin()     + index);
	asteroidSpinSpeeds.erase(asteroidSpinSpeeds.begin() + index);
	asteroidHP.erase        (asteroidHP.begin()         + index);
	asteroidScales.erase    (asteroidScales.begin()     + index);
}

void Enemy::RemoveSmallAsteroid(int index) {
	SpawnKillDebris(smallAsteroidPositions[index], Vec3(0.6f, 0.6f, 0.6f), 4);
	smallAsteroidPositions.erase (smallAsteroidPositions.begin()  + index);
	smallAsteroidAngles.erase    (smallAsteroidAngles.begin()     + index);
	smallAsteroidSpinSpeeds.erase(smallAsteroidSpinSpeeds.begin() + index);
	smallAsteroidHP.erase        (smallAsteroidHP.begin()         + index);
	smallAsteroidScales.erase    (smallAsteroidScales.begin()     + index);
}

void Enemy::RemoveBot01(int index) {
	SpawnKillDebris(bot01Positions[index], Vec3(1.0f, 0.2f, 0.2f), 6);
	bot01Positions.erase  (bot01Positions.begin()   + index);
	bot01YVelocities.erase(bot01YVelocities.begin() + index);
	bot01HP.erase         (bot01HP.begin()          + index);
	bot01HitTimers.erase  (bot01HitTimers.begin()   + index);
}

void Enemy::Reset() {
	asteroidPositions.clear();
	asteroidAngles.clear();
	asteroidSpinSpeeds.clear();
	asteroidHP.clear();
	asteroidScales.clear();
	smallAsteroidPositions.clear();
	smallAsteroidAngles.clear();
	smallAsteroidSpinSpeeds.clear();
	smallAsteroidHP.clear();
	smallAsteroidScales.clear();
	bot01Positions.clear();
	bot01YVelocities.clear();
	bot01HP.clear();
	bot01HitTimers.clear();
	debris.clear();
	asteroidSpawnTimer = 0.0f;
	smallAsteroidSpawnTimer = 0.0f;
	bot01SpawnTimer = 0.0f;
	totalTime = 0.0f;
}