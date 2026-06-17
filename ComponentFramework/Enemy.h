#pragma once
#ifndef ENEMY_H
#define ENEMY_H

#include "Mesh.h"
#include "Shader.h"
#include <Matrix.h>
#include <Vector.h>
#include <vector>

using namespace MATH;

enum class EnemyType {
	ASTEROID,	// wave 1
	BOT01,		// wave 2
};

struct Debris {
	Vec3  pos;
	Vec3  vel;
	Vec3  color;
	float angle;
	float spinSpeed;
	float lifetime;
	float maxLifetime;
};

class Enemy {
private:
	// Meshes
	Mesh* asteroidMesh;
	Mesh* bot01Mesh;
	Mesh* bot01ThrustMesh;

	float thrustTimer;

	// Positions
	std::vector<Vec3> asteroidPositions;
	std::vector<Vec3> smallAsteroidPositions;
	std::vector<Vec3> bot01Positions;

	// Spin (feature 1)
	std::vector<float> asteroidAngles;
	std::vector<float> asteroidSpinSpeeds;
	std::vector<float> smallAsteroidAngles;
	std::vector<float> smallAsteroidSpinSpeeds;

	// Bot01 Y-axis steering (feature 3)
	std::vector<float> bot01YVelocities;
	float bot01SteerForce;
	float bot01YDamping;
	float bot01YMaxSpeed;

	// Debris (feature 5)
	std::vector<Debris> debris;
	void SpawnDebris(const Vec3& pos, const Vec3& color);

	// speed
	float asteroidSpeed;
	float smallAsteroidSpeed;
	float bot01Speed;

	// Spawn timers
	float asteroidSpawnTimer;
	float asteroidSpawnInterval;
	float smallAsteroidSpawnTimer;
	float smallAsteroidSpawnInterval;
	float bot01SpawnTimer;
	float bot01SpawnInterval;

	// wave timer
	float totalTime;

public:
	Enemy();
	~Enemy();

	bool OnCreate(const char* asteroidFile,
				  const char* bot01File);
	void OnDestroy();
	void Update(float deltaTime, float playerY = 0.0f);
	void Render(Shader* shader,
		const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const;

	// Getters for collision
	std::vector<Vec3>& GetAsteroidPositions()      { return asteroidPositions; }
	std::vector<Vec3>& GetSmallAsteroidPositions() { return smallAsteroidPositions; }
	std::vector<Vec3>& GetBot01Positions()         { return bot01Positions; }

	// Getters for missile guidance (enemies only ever move along -x)
	float GetAsteroidSpeed()      const { return asteroidSpeed; }
	float GetSmallAsteroidSpeed() const { return smallAsteroidSpeed; }
	float GetBot01Speed()         const { return bot01Speed; }

	// Remove by index
	void RemoveAsteroid(int index);
	void RemoveSmallAsteroid(int index);
	void RemoveBot01(int index);

	// Reset
	void Reset();

};
#endif // !ENEMY_H
