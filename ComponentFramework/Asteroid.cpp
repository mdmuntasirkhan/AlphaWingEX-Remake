#include "Asteroid.h"
#include "GameConstants.h"
#include <MMath.h>
#include <glew.h>
#include <cstdlib>
#include <cmath>
#include <iostream>

Asteroid::Asteroid() :
	asteroidMesh{ nullptr },
	asteroidSpeed{ 0.9f },
	smallAsteroidSpeed{ 1.6f },
	asteroidSpawnTimer{ 0.0f },
	asteroidSpawnInterval{ 3.2f },
	smallAsteroidSpawnTimer{ 0.0f },
	smallAsteroidSpawnInterval{ 2.0f },
	spawningEnabled{ true }
{
}

Asteroid::~Asteroid() {}

bool Asteroid::OnCreate(const char* meshFile) {
	asteroidMesh = new Mesh(meshFile);
	if (asteroidMesh->OnCreate() == false) {
		std::cout << "Asteroid mesh not found!\n";
		return false;
	}
	return true;
}

void Asteroid::OnDestroy() {
	if (asteroidMesh) {
		asteroidMesh->OnDestroy();
		delete asteroidMesh;
		asteroidMesh = nullptr;
	}
	asteroidPositions.clear();
	asteroidAngles.clear();
	asteroidSpinSpeeds.clear();
	asteroidHP.clear();
	asteroidScales.clear();
	asteroidKnockVelX.clear();
	asteroidKnockVelY.clear();
	smallAsteroidPositions.clear();
	smallAsteroidAngles.clear();
	smallAsteroidSpinSpeeds.clear();
	smallAsteroidHP.clear();
	smallAsteroidScales.clear();
	smallAsteroidKnockVelX.clear();
	smallAsteroidKnockVelY.clear();
	debris.clear();
}

void Asteroid::Update(float deltaTime, float /*playerX*/, float /*playerY*/) {
	// Large asteroids
	if (spawningEnabled) asteroidSpawnTimer += deltaTime;
	if (spawningEnabled && asteroidSpawnTimer >= asteroidSpawnInterval) {
		asteroidSpawnTimer = 0.0f;
		float randomY = ((rand() % 25) - 12) * 0.5f; // ±6.0, matches visible Y range
		asteroidPositions.push_back(Vec3(GameConst::kSpawnX, randomY, GameConst::kWorldZ));
		asteroidAngles.push_back(0.0f);
		asteroidSpinSpeeds.push_back((float)((rand() % 121) - 60));
		asteroidHP.push_back(kLargeHP);
		asteroidScales.push_back(kLargeScale);
		asteroidKnockVelX.push_back(0.0f);
		asteroidKnockVelY.push_back(0.0f);
	}
	for (int i = 0; i < (int)asteroidPositions.size(); i++) {
		asteroidPositions[i].x += asteroidKnockVelX[i] * deltaTime;
		asteroidPositions[i].y += asteroidKnockVelY[i] * deltaTime;
		asteroidKnockVelX[i] *= expf(-kKnockbackDecay * deltaTime);
		asteroidKnockVelY[i] *= expf(-kKnockbackDecay * deltaTime);
		if (fabsf(asteroidKnockVelX[i]) < 0.01f) asteroidKnockVelX[i] = 0.0f;
		if (fabsf(asteroidKnockVelY[i]) < 0.01f) asteroidKnockVelY[i] = 0.0f;
		asteroidPositions[i].x -= asteroidSpeed * deltaTime;
		asteroidAngles[i]      += asteroidSpinSpeeds[i] * deltaTime;
	}
	for (int i = (int)asteroidPositions.size() - 1; i >= 0; i--) {
		if (asteroidPositions[i].x < GameConst::kCullX) {
			asteroidPositions.erase (asteroidPositions.begin()  + i);
			asteroidAngles.erase    (asteroidAngles.begin()     + i);
			asteroidSpinSpeeds.erase(asteroidSpinSpeeds.begin() + i);
			asteroidHP.erase        (asteroidHP.begin()         + i);
			asteroidScales.erase    (asteroidScales.begin()     + i);
			asteroidKnockVelX.erase (asteroidKnockVelX.begin()  + i);
			asteroidKnockVelY.erase (asteroidKnockVelY.begin()  + i);
		}
	}

	// Small asteroids
	if (spawningEnabled) smallAsteroidSpawnTimer += deltaTime;
	if (spawningEnabled && smallAsteroidSpawnTimer >= smallAsteroidSpawnInterval) {
		smallAsteroidSpawnTimer = 0.0f;
		float randomY = ((rand() % 25) - 12) * 0.5f; // ±6.0, matches visible Y range
		smallAsteroidPositions.push_back(Vec3(GameConst::kSpawnX, randomY, GameConst::kWorldZ));
		smallAsteroidAngles.push_back(0.0f);
		smallAsteroidSpinSpeeds.push_back((float)((rand() % 181) - 90));
		smallAsteroidHP.push_back(kSmallHP);
		smallAsteroidScales.push_back(kSmallScale);
		smallAsteroidKnockVelX.push_back(0.0f);
		smallAsteroidKnockVelY.push_back(0.0f);
	}
	for (int i = 0; i < (int)smallAsteroidPositions.size(); i++) {
		smallAsteroidPositions[i].x += smallAsteroidKnockVelX[i] * deltaTime;
		smallAsteroidPositions[i].y += smallAsteroidKnockVelY[i] * deltaTime;
		smallAsteroidKnockVelX[i] *= expf(-8.0f * deltaTime);
		smallAsteroidKnockVelY[i] *= expf(-8.0f * deltaTime);
		if (fabsf(smallAsteroidKnockVelX[i]) < 0.01f) smallAsteroidKnockVelX[i] = 0.0f;
		if (fabsf(smallAsteroidKnockVelY[i]) < 0.01f) smallAsteroidKnockVelY[i] = 0.0f;
		smallAsteroidPositions[i].x -= smallAsteroidSpeed * deltaTime;
		smallAsteroidAngles[i]      += smallAsteroidSpinSpeeds[i] * deltaTime;
	}
	for (int i = (int)smallAsteroidPositions.size() - 1; i >= 0; i--) {
		if (smallAsteroidPositions[i].x < GameConst::kCullX) {
			smallAsteroidPositions.erase (smallAsteroidPositions.begin()  + i);
			smallAsteroidAngles.erase    (smallAsteroidAngles.begin()     + i);
			smallAsteroidSpinSpeeds.erase(smallAsteroidSpinSpeeds.begin() + i);
			smallAsteroidHP.erase        (smallAsteroidHP.begin()         + i);
			smallAsteroidScales.erase    (smallAsteroidScales.begin()     + i);
			smallAsteroidKnockVelX.erase (smallAsteroidKnockVelX.begin()  + i);
			smallAsteroidKnockVelY.erase (smallAsteroidKnockVelY.begin()  + i);
		}
	}

	// Debris
	for (int i = (int)debris.size() - 1; i >= 0; i--) {
		debris[i].pos      += debris[i].vel * deltaTime;
		debris[i].angle    += debris[i].spinSpeed * deltaTime;
		debris[i].lifetime -= deltaTime;
		if (debris[i].lifetime <= 0.0f)
			debris.erase(debris.begin() + i);
	}
}

void Asteroid::Render(Shader* shader,
	const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const {
	glUniformMatrix4fv(shader->GetUniformID("projectionMatrix"), 1, GL_FALSE, projectionMatrix);
	glUniformMatrix4fv(shader->GetUniformID("viewMatrix"), 1, GL_FALSE, viewMatrix);

	// Large asteroids — orange
	glUniform4f(shader->GetUniformID("color"), 1.0f, 0.3f, 0.0f, 1.0f);
	for (int i = 0; i < (int)asteroidPositions.size(); i++) {
		float s = asteroidScales[i];
		Matrix4 m = MMath::translate(asteroidPositions[i]) *
		            MMath::rotate(asteroidAngles[i], Vec3(0.0f, 0.0f, 1.0f)) *
		            MMath::scale(s, s, s);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
		asteroidMesh->Render();
	}

	// Small asteroids — grey
	glUniform4f(shader->GetUniformID("color"), 0.6f, 0.6f, 0.6f, 1.0f);
	for (int i = 0; i < (int)smallAsteroidPositions.size(); i++) {
		float s = smallAsteroidScales[i];
		Matrix4 m = MMath::translate(smallAsteroidPositions[i]) *
		            MMath::rotate(smallAsteroidAngles[i], Vec3(0.0f, 0.0f, 1.0f)) *
		            MMath::scale(s, s, s);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
		asteroidMesh->Render();
	}

	// Debris
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

bool Asteroid::DamageAsteroid(int index, int amount) {
	asteroidHP[index] -= amount;
	for (int i = 0; i < amount; i++) asteroidScales[index] *= 0.88f;
	SpawnHitDebris(asteroidPositions[index], Vec3(1.0f, 0.5f, 0.0f), amount > 1 ? 5 : 2);

	if (asteroidHP[index] <= 0) {
		SpawnKillDebris(asteroidPositions[index], Vec3(1.0f, 0.5f, 0.0f), amount > 1 ? 10 : 6);
		asteroidPositions.erase (asteroidPositions.begin()  + index);
		asteroidAngles.erase    (asteroidAngles.begin()     + index);
		asteroidSpinSpeeds.erase(asteroidSpinSpeeds.begin() + index);
		asteroidHP.erase        (asteroidHP.begin()         + index);
		asteroidScales.erase    (asteroidScales.begin()     + index);
		asteroidKnockVelX.erase (asteroidKnockVelX.begin()  + index);
		asteroidKnockVelY.erase (asteroidKnockVelY.begin()  + index);
		return true;
	}
	return false;
}

bool Asteroid::DamageSmallAsteroid(int index, int amount) {
	smallAsteroidHP[index] -= amount;
	for (int i = 0; i < amount; i++) smallAsteroidScales[index] *= 0.82f;
	SpawnHitDebris(smallAsteroidPositions[index], Vec3(0.6f, 0.6f, 0.6f), amount > 1 ? 4 : 1);

	if (smallAsteroidHP[index] <= 0) {
		SpawnKillDebris(smallAsteroidPositions[index], Vec3(0.6f, 0.6f, 0.6f), amount > 1 ? 8 : 4);
		smallAsteroidPositions.erase (smallAsteroidPositions.begin()  + index);
		smallAsteroidAngles.erase    (smallAsteroidAngles.begin()     + index);
		smallAsteroidSpinSpeeds.erase(smallAsteroidSpinSpeeds.begin() + index);
		smallAsteroidHP.erase        (smallAsteroidHP.begin()         + index);
		smallAsteroidScales.erase    (smallAsteroidScales.begin()     + index);
		smallAsteroidKnockVelX.erase (smallAsteroidKnockVelX.begin()  + index);
		smallAsteroidKnockVelY.erase (smallAsteroidKnockVelY.begin()  + index);
		return true;
	}
	return false;
}

void Asteroid::RemoveAsteroid(int index) {
	SpawnKillDebris(asteroidPositions[index], Vec3(1.0f, 0.5f, 0.0f), 6);
	asteroidPositions.erase (asteroidPositions.begin()  + index);
	asteroidAngles.erase    (asteroidAngles.begin()     + index);
	asteroidSpinSpeeds.erase(asteroidSpinSpeeds.begin() + index);
	asteroidHP.erase        (asteroidHP.begin()         + index);
	asteroidScales.erase    (asteroidScales.begin()     + index);
	asteroidKnockVelX.erase (asteroidKnockVelX.begin()  + index);
	asteroidKnockVelY.erase (asteroidKnockVelY.begin()  + index);
}

void Asteroid::RemoveSmallAsteroid(int index) {
	SpawnKillDebris(smallAsteroidPositions[index], Vec3(0.6f, 0.6f, 0.6f), 4);
	smallAsteroidPositions.erase (smallAsteroidPositions.begin()  + index);
	smallAsteroidAngles.erase    (smallAsteroidAngles.begin()     + index);
	smallAsteroidSpinSpeeds.erase(smallAsteroidSpinSpeeds.begin() + index);
	smallAsteroidHP.erase        (smallAsteroidHP.begin()         + index);
	smallAsteroidScales.erase    (smallAsteroidScales.begin()     + index);
	smallAsteroidKnockVelX.erase (smallAsteroidKnockVelX.begin()  + index);
	smallAsteroidKnockVelY.erase (smallAsteroidKnockVelY.begin()  + index);
}

void Asteroid::Reset() {
	asteroidPositions.clear();
	asteroidAngles.clear();
	asteroidSpinSpeeds.clear();
	asteroidHP.clear();
	asteroidScales.clear();
	asteroidKnockVelX.clear();
	asteroidKnockVelY.clear();
	smallAsteroidPositions.clear();
	smallAsteroidAngles.clear();
	smallAsteroidSpinSpeeds.clear();
	smallAsteroidHP.clear();
	smallAsteroidScales.clear();
	smallAsteroidKnockVelX.clear();
	smallAsteroidKnockVelY.clear();
	debris.clear();
	asteroidSpawnTimer      = 0.0f;
	smallAsteroidSpawnTimer = 0.0f;
}
