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
	// Regular bullet
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
	std::vector<float> missileLaunchTimers;		// flies straight until this exceeds missileLaunchDuration
	std::vector<float>   missileLifetimers;		// missile dies on timeout, not screen edge

	// Regular bullet fire rate
	float fireCooldown;
	float fireCooldownTimer;

	// Homing Missiles guidance tuning
	float missileLaunchDuration;	// seconds of straight flight before homing activates
	float missileSpeed;
	float missileNavigationGain;    // "N" in the PN law, higher = more aggressive turns
	float missileMaxLateralAccel;   // clamp so PN can't whip the missile around instantly
	float missileTerminalRange;     // distance at which terminal speed kicks in

	// Three-phase speed profile, burst, cruise, terminal
	float missileLaunchSpeed;       // initial burst speed (fast, exciting)
	float missileCruiseSpeed;       // slow hunting speed after decel (accurate, never misses)
	float missileTerminalSpeed;     // final sprint onto the target
	float missileDecelDuration;     // seconds to decelerate from launch to cruise speed
	float missileMaxLifetime;
	
	// Homing missile supply system
	int   missileCount;
	int   maxMissiles;
	float missileReloadTimer;
	float missileReloadInterval;
	float missileCooldown;
	float missileCooldownTimer;

	static constexpr float kBulletSpreadY = 0.004f;  // Y spread per random unit (±0.2 u/s)

	// Re-acquire a target after the locked one is destroyed mid-flight. Priority is Bot02 > Bot01 > Asteroid. Within a tier, picks the nearest one.
	bool FindNearestTarget(const Vec3& fromPosition,
		const std::vector<Vec3>& asteroidPositions,
		const std::vector<Vec3>& bot01Positions,
		const std::vector<Vec3>& bot02Positions,
		MissileTargetType& outType, int& outIndex) const;

public:
	Bullet();
	~Bullet();

	// Lifecycle
	bool OnCreate(const char* bulletMeshFile, const char* missileMeshFile);
	void OnDestroy();
	void Update(float deltaTime,
				const std::vector<Vec3>& asteroidPositions, const Vec3& asteroidVelocity,
				const std::vector<Vec3>& bot01Positions,   const Vec3& bot01Velocity,
				const std::vector<Vec3>& bot02Positions,   const Vec3& bot02Velocity);
	void Render(Shader* shader,
				const Matrix4& projectionMatrix,
				const Matrix4& viewMatrix) const;

	// Spawn, Regular bullet and Homing missile
	void Spawn(Vec3 position);
	void SpawnHoming(Vec3 position, MissileTargetType targetType, int targetIndex);

	// Getters for collision
	std::vector<Vec3>& GetPositions()         { return positions; }
	std::vector<Vec3>& GetMissilePositions()  { return missilePositions; }
	std::vector<Vec3>& GetMissileVelocities() { return missileVelocities; }

	// Remove by index
	void RemoveAt(int index);
	void RemoveMissileAt(int index);

	// HUD queries
	int   GetMissileCount()          const { return missileCount; }
	int   GetMaxMissiles()           const { return maxMissiles; }
	float GetReloadFraction()        const { return (missileReloadInterval > 0.0f) ? missileReloadTimer / missileReloadInterval : 0.0f; }
	float GetMissileCooldownFraction() const { return (missileCooldown > 0.0f) ? missileCooldownTimer / missileCooldown : 0.0f; }

};

#endif // BULLET_H
