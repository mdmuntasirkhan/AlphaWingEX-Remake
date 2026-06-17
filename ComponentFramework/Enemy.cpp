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
	asteroidSpeed{ 1.4f },
	smallAsteroidSpeed{ 2.6f },
	bot01Speed{ 2.0f },
	asteroidSpawnTimer{ 0.0f },
	asteroidSpawnInterval{ 2.2f },
	smallAsteroidSpawnTimer{ 0.0f },
	smallAsteroidSpawnInterval{ 1.3f },
	bot01SpawnTimer{ 0.0f },
	bot01SpawnInterval{ 3.0f },
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
	smallAsteroidPositions.clear();
	smallAsteroidAngles.clear();
	smallAsteroidSpinSpeeds.clear();
	bot01Positions.clear();
	bot01YVelocities.clear();
	debris.clear();
}

void Enemy::Update(float deltaTime, float playerY) {
	totalTime += deltaTime;
	thrustTimer += deltaTime;

	// --- Large asteroids ---
	asteroidSpawnTimer += deltaTime;
	if (asteroidSpawnTimer >= asteroidSpawnInterval) {
		asteroidSpawnTimer = 0.0f;
		float randomY = ((rand() % 10) - 5) * 0.5f;
		asteroidPositions.push_back(Vec3(15.0f, randomY, -10.0f));
		asteroidAngles.push_back(0.0f);
		asteroidSpinSpeeds.push_back((float)((rand() % 121) - 60)); // -60..+60 deg/s
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
		}
	}

	// --- Small asteroids ---
	smallAsteroidSpawnTimer += deltaTime;
	if (smallAsteroidSpawnTimer >= smallAsteroidSpawnInterval) {
		smallAsteroidSpawnTimer = 0.0f;
		float randomY = ((rand() % 14) - 7) * 0.5f;
		smallAsteroidPositions.push_back(Vec3(15.0f, randomY, -10.0f));
		smallAsteroidAngles.push_back(0.0f);
		smallAsteroidSpinSpeeds.push_back((float)((rand() % 181) - 90)); // -90..+90 deg/s
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
		}
	}

	// --- Bot01 (wave 2, after 30 s) ---
	if (totalTime > 30.0f) {
		bot01SpawnTimer += deltaTime;
		if (bot01SpawnTimer >= bot01SpawnInterval) {
			bot01SpawnTimer = 0.0f;
			float randomY = ((rand() % 10) - 5) * 0.5f;
			bot01Positions.push_back(Vec3(15.0f, randomY, -10.0f));
			bot01YVelocities.push_back(0.0f);
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
			bot01Positions.erase(bot01Positions.begin() + i);
			bot01YVelocities.erase(bot01YVelocities.begin() + i);
		}
	}

	// --- Debris ---
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

	// Large asteroids — ORANGE, spinning
	glUniform4f(shader->GetUniformID("color"), 1.0f, 0.3f, 0.0f, 1.0f);
	for (int i = 0; i < (int)asteroidPositions.size(); i++) {
		Matrix4 m = MMath::translate(asteroidPositions[i]) *
		            MMath::rotate(asteroidAngles[i], Vec3(0.0f, 0.0f, 1.0f)) *
		            MMath::scale(0.4f, 0.4f, 0.4f);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
		asteroidMesh->Render();
	}

	// Small asteroids — GREY, spinning faster
	glUniform4f(shader->GetUniformID("color"), 0.6f, 0.6f, 0.6f, 1.0f);
	for (int i = 0; i < (int)smallAsteroidPositions.size(); i++) {
		Matrix4 m = MMath::translate(smallAsteroidPositions[i]) *
		            MMath::rotate(smallAsteroidAngles[i], Vec3(0.0f, 0.0f, 1.0f)) *
		            MMath::scale(0.2f, 0.2f, 0.2f);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
		asteroidMesh->Render();
	}

	// Bot01 — RED
	glUniform4f(shader->GetUniformID("color"), 1.0f, 0.1f, 0.1f, 1.0f);
	for (int i = 0; i < (int)bot01Positions.size(); i++) {
		Matrix4 m = MMath::translate(bot01Positions[i]) *
			MMath::rotate(180.0f, Vec3(0.0f, 1.0f, 0.0f)) *
			MMath::scale(0.17f, 0.17f, 0.17f);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
		bot01Mesh->Render();
	}

	// Bot01 thrust — additive blue electric pulse (faster than player's orange)
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
			Matrix4 m = MMath::translate(debris[i].pos) *
			            MMath::rotate(debris[i].angle, Vec3(0.0f, 0.0f, 1.0f)) *
			            MMath::scale(0.06f, 0.06f, 0.06f);
			glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
			asteroidMesh->Render();
		}
		glUniform1f(shader->GetUniformID("emissive"), 0.0f);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}
}

void Enemy::SpawnDebris(const Vec3& pos, const Vec3& color) {
	const int count = 4;
	for (int i = 0; i < count; i++) {
		Debris d;
		d.pos   = pos;
		d.color = color;
		float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
		float spd   = 2.0f + (rand() % 40) * 0.1f;   // 2..6 units/s
		d.vel       = Vec3(cosf(angle) * spd, sinf(angle) * spd, 0.0f);
		d.angle     = (float)(rand() % 360);
		d.spinSpeed = (float)((rand() % 361) - 180);  // -180..+180 deg/s
		d.maxLifetime = 0.6f + (rand() % 40) * 0.01f; // 0.6..1.0 s
		d.lifetime    = d.maxLifetime;
		debris.push_back(d);
	}
}

void Enemy::RemoveAsteroid(int index) {
	SpawnDebris(asteroidPositions[index], Vec3(1.0f, 0.5f, 0.0f));
	asteroidPositions.erase(asteroidPositions.begin() + index);
	asteroidAngles.erase(asteroidAngles.begin() + index);
	asteroidSpinSpeeds.erase(asteroidSpinSpeeds.begin() + index);
}

void Enemy::RemoveSmallAsteroid(int index) {
	SpawnDebris(smallAsteroidPositions[index], Vec3(0.6f, 0.6f, 0.6f));
	smallAsteroidPositions.erase(smallAsteroidPositions.begin() + index);
	smallAsteroidAngles.erase(smallAsteroidAngles.begin() + index);
	smallAsteroidSpinSpeeds.erase(smallAsteroidSpinSpeeds.begin() + index);
}

void Enemy::RemoveBot01(int index) {
	SpawnDebris(bot01Positions[index], Vec3(1.0f, 0.2f, 0.2f));
	bot01Positions.erase(bot01Positions.begin() + index);
	bot01YVelocities.erase(bot01YVelocities.begin() + index);
}

void Enemy::Reset() {
	asteroidPositions.clear();
	asteroidAngles.clear();
	asteroidSpinSpeeds.clear();
	smallAsteroidPositions.clear();
	smallAsteroidAngles.clear();
	smallAsteroidSpinSpeeds.clear();
	bot01Positions.clear();
	bot01YVelocities.clear();
	debris.clear();
	asteroidSpawnTimer = 0.0f;
	smallAsteroidSpawnTimer = 0.0f;
	bot01SpawnTimer = 0.0f;
	totalTime = 0.0f;
}