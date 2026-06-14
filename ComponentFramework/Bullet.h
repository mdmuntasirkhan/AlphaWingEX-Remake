#pragma once
#ifndef BULLET_H
#define BULLET_H

#include "Mesh.h"
#include "Shader.h"
#include <Matrix.h>
#include <Vector.h>
#include <vector>

using namespace MATH;

class Bullet {
private:
	// Regular one
	Mesh* mesh;
	std::vector<Vec3> positions;
	float speed;

	// Homing Missile
	Mesh* missileMesh;
	std::vector<Vec3> missilePositions;
	Vec3* homingTarget;
	float missileSpeed;

	// Homing missile supply system
	int missileCount;
	int maxMissiles;
	float missileReloadTimer;
	float missileReloadInterval;

public:
	Bullet();
	~Bullet();

	bool OnCreate(const char* bulletMeshFile, const char* missileMeshFile);
	void OnDestroy();
	void Update(float deltaTime);
	void Render(Shader* shader,
		const Matrix4& projectionMatrix,
		const Matrix4& viewMatrix) const;

	// Regular one
	void Spawn(Vec3 position);

	// Homing missile - right click
	void SpawnHoming(Vec3 position, Vec3* target);

	// Getters for collision
	std::vector<Vec3>& GetPositions() { return positions; }
	std::vector<Vec3>& GetMissilePositions() { return missilePositions; }

	// Remove by index
	void RemoveAt(int index);
	void RemoveMissileAt(int index);

};

#endif // !BULLET_H
