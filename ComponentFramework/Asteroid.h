#pragma once
#ifndef ASTEROID_H
#define ASTEROID_H

#include "Enemy.h"
#include <vector>

// Manages two parallel pools of scrolling hazards: large orange asteroids and small grey ones.
// Both pools share a single mesh. Supports physics-based knockback, hit/kill debris, and
// configurable spawn rates driven by the level script via SetSpawnRates().
class Asteroid : public Enemy {
private:
	Mesh* asteroidMesh;

	static constexpr int   kLargeHP        = 6;     // hits to kill a large asteroid
	static constexpr int   kSmallHP        = 3;     // hits to kill a small asteroid
	static constexpr float kLargeScale     = 0.4f;  // initial uniform scale for large asteroids
	static constexpr float kSmallScale     = 0.2f;  // initial uniform scale for small asteroids
	static constexpr float kKnockbackDecay = 8.0f;  // exponential decay rate for knockback velocity

	std::vector<Vec3>  asteroidPositions;
	std::vector<Vec3>  smallAsteroidPositions;
	std::vector<float> asteroidAngles;
	std::vector<float> asteroidSpinSpeeds;
	std::vector<float> smallAsteroidAngles;
	std::vector<float> smallAsteroidSpinSpeeds;
	std::vector<int>   asteroidHP;
	std::vector<float> asteroidScales;
	std::vector<float> asteroidKnockVelX;
	std::vector<float> asteroidKnockVelY;
	std::vector<int>   smallAsteroidHP;
	std::vector<float> smallAsteroidScales;
	std::vector<float> smallAsteroidKnockVelX;
	std::vector<float> smallAsteroidKnockVelY;

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
	void PushAsteroid(int index, float dx, float dy) {
		if (index >= 0 && index < (int)asteroidKnockVelX.size()) {
			asteroidKnockVelX[index] += dx;
			asteroidKnockVelY[index] += dy;
		}
	}
	void PushSmallAsteroid(int index, float dx, float dy) {
		if (index >= 0 && index < (int)smallAsteroidKnockVelX.size()) {
			smallAsteroidKnockVelX[index] += dx;
			smallAsteroidKnockVelY[index] += dy;
		}
	}
};

#endif // ASTEROID_H
