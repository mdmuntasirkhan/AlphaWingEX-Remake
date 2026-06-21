#pragma once
#ifndef BULLET_H
#define BULLET_H

#include "Mesh.h"
#include "Shader.h"
#include <Matrix.h>
#include <Vector.h>
#include <vector>

using namespace MATH;

// What a homing missile is currently locked onto — ordered by threat (highest first)
enum class MissileTargetType {
	NONE,
	ASTEROID,
	BOT01,
	BOT02
};

class Bullet {
private:
	// Regular one
	Mesh* mesh;
	std::vector<Vec3>  positions;
	std::vector<float> bulletYVelocities;
	float speed;

	// Homing Missile
	Mesh* missileMesh;
	std::vector<Vec3> missilePositions;
	std::vector<Vec3> missileVelocities; // current heading*speed - drives PN steering
	std::vector<MissileTargetType> missileTargetTypes;
	std::vector<int> missileTargetIndices;
	std::vector<float> missileLaunchTimers;
	float missileLaunchDuration; // fly straight forward for this long before homing kicks in
	float missileSpeed;

	// Proportional navigation guidance tuning
	float missileNavigationGain;	 // "N" in the PN law - higher = more aggressive turns
	float missileMaxLateralAccel;	 // clamp so PN can't whip the missile around instantly
	float missileTerminalRange;	 // once this close to the real target, floor the throttle
	float missileTerminalSpeedMultiplier;

	// Homing missile supply system
	int missileCount;
	int maxMissiles;
	float missileReloadTimer;
	float missileReloadInterval;

	// Regular bullet fire rate limit
	float fireCooldown;
	float fireCooldownTimer;

	// Re-acquire a target after the locked one is destroyed mid-flight.
	// Priority: Bot02 > Bot01 > Asteroid. Within a tier, picks the nearest one.
	bool FindNearestTarget(const Vec3& fromPosition,
		const std::vector<Vec3>& asteroidPositions,
		const std::vector<Vec3>& bot01Positions,
		const std::vector<Vec3>& bot02Positions,
		MissileTargetType& outType, int& outIndex) const;

public:
	Bullet();
	~Bullet();

	bool OnCreate(const char* bulletMeshFile, const char* missileMeshFile);
	void OnDestroy();
	void Update(float deltaTime,
		const std::vector<Vec3>& asteroidPositions, const Vec3& asteroidVelocity,
		const std::vector<Vec3>& bot01Positions,   const Vec3& bot01Velocity,
		const std::vector<Vec3>& bot02Positions,   const Vec3& bot02Velocity);
	void Render(Shader* shader,
		const Matrix4& projectionMatrix,
		const Matrix4& viewMatrix) const;

	// Regular one
	void Spawn(Vec3 position);

	// Homing missile - right click
	void SpawnHoming(Vec3 position, MissileTargetType targetType, int targetIndex);

	// Getters for collision
	std::vector<Vec3>& GetPositions()         { return positions; }
	std::vector<Vec3>& GetMissilePositions()  { return missilePositions; }
	std::vector<Vec3>& GetMissileVelocities() { return missileVelocities; }

	// Remove by index
	void RemoveAt(int index);
	void RemoveMissileAt(int index);

	// HUD query
	int GetMissileCount() const { return missileCount; }
	int GetMaxMissiles()  const { return maxMissiles; }

};

#endif // !BULLET_H
