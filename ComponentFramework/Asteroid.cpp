#include "Asteroid.h"
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
	smallAsteroidSpawnInterval{ 2.0f }
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
	smallAsteroidPositions.clear();
	smallAsteroidAngles.clear();
	smallAsteroidSpinSpeeds.clear();
	smallAsteroidHP.clear();
	smallAsteroidScales.clear();
	debris.clear();
}

void Asteroid::Update(float deltaTime, float /*playerY*/) {
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

bool Asteroid::DamageAsteroid(int index) {
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

bool Asteroid::DamageSmallAsteroid(int index) {
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

void Asteroid::RemoveAsteroid(int index) {
	SpawnKillDebris(asteroidPositions[index], Vec3(1.0f, 0.5f, 0.0f), 6);
	asteroidPositions.erase (asteroidPositions.begin()  + index);
	asteroidAngles.erase    (asteroidAngles.begin()     + index);
	asteroidSpinSpeeds.erase(asteroidSpinSpeeds.begin() + index);
	asteroidHP.erase        (asteroidHP.begin()         + index);
	asteroidScales.erase    (asteroidScales.begin()     + index);
}

void Asteroid::RemoveSmallAsteroid(int index) {
	SpawnKillDebris(smallAsteroidPositions[index], Vec3(0.6f, 0.6f, 0.6f), 4);
	smallAsteroidPositions.erase (smallAsteroidPositions.begin()  + index);
	smallAsteroidAngles.erase    (smallAsteroidAngles.begin()     + index);
	smallAsteroidSpinSpeeds.erase(smallAsteroidSpinSpeeds.begin() + index);
	smallAsteroidHP.erase        (smallAsteroidHP.begin()         + index);
	smallAsteroidScales.erase    (smallAsteroidScales.begin()     + index);
}

void Asteroid::Reset() {
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
	debris.clear();
	asteroidSpawnTimer      = 0.0f;
	smallAsteroidSpawnTimer = 0.0f;
}
