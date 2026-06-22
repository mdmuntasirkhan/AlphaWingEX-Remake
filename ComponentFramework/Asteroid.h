#pragma once
#ifndef ASTEROID_H
#define ASTEROID_H

#include "Enemy.h"
#include <vector>

class Asteroid : public Enemy {
private:
	Mesh* asteroidMesh;

	std::vector<Vec3>  asteroidPositions;
	std::vector<Vec3>  smallAsteroidPositions;
	std::vector<float> asteroidAngles;
	std::vector<float> asteroidSpinSpeeds;
	std::vector<float> smallAsteroidAngles;
	std::vector<float> smallAsteroidSpinSpeeds;
	std::vector<int>   asteroidHP;
	std::vector<float> asteroidScales;
	std::vector<int>   smallAsteroidHP;
	std::vector<float> smallAsteroidScales;

	float asteroidSpeed;
	float smallAsteroidSpeed;
	float asteroidSpawnTimer;
	float asteroidSpawnInterval;
	float smallAsteroidSpawnTimer;
	float smallAsteroidSpawnInterval;
	bool  spawningEnabled;

public:
	Asteroid();
	~Asteroid();

	bool OnCreate(const char* meshFile);
	void OnDestroy() override;
	void Update(float deltaTime, float playerX = 0.0f, float playerY = 0.0f) override;
	void Render(Shader* shader,
		const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const override;
	void Reset() override;

	std::vector<Vec3>& GetAsteroidPositions()      { return asteroidPositions; }
	std::vector<Vec3>& GetSmallAsteroidPositions() { return smallAsteroidPositions; }
	float GetAsteroidSpeed()      const { return asteroidSpeed; }
	float GetSmallAsteroidSpeed() const { return smallAsteroidSpeed; }

	void SetSpawningEnabled(bool enabled) { spawningEnabled = enabled; }
	void SetSpawnRates(float largeInterval, float smallInterval) {
		asteroidSpawnInterval      = largeInterval;
		smallAsteroidSpawnInterval = smallInterval;
	}

	bool DamageAsteroid(int index, int amount = 1);
	bool DamageSmallAsteroid(int index, int amount = 1);
	void RemoveAsteroid(int index);
	void RemoveSmallAsteroid(int index);
};

#endif // ASTEROID_H
