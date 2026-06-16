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

class Enemy {
private:
	// Meshes
	Mesh* asteroidMesh;
	Mesh* bot01Mesh;

	// Positions
	std::vector<Vec3> asteroidPositions;
	std::vector<Vec3> bot01Positions;

	// speed
	float asteroidSpeed;
	float bot01Speed;

	// Spawn timers
	float asteroidSpawnTimer;
	float asteroidSpawnInterval;
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
	void Update(float deltaTime);
	void Render(Shader* shader,
		const Matrix4& projectionMatrix,
		const Matrix4& viewMatrix) const;

	// Getters for collision
	std::vector<Vec3>& GetAsteroidPositions() { return asteroidPositions; }
	std::vector<Vec3>& GetBot01Positions() { return bot01Positions; }

	// Getters for missile guidance (enemies only ever move along -x)
	float GetAsteroidSpeed() const { return asteroidSpeed; }
	float GetBot01Speed() const { return bot01Speed; }

	// Remove by index
	void RemoveAsteroid(int index);
	void RemoveBot01(int index);

	// Reset
	void Reset();

};
#endif // !ENEMY_H
