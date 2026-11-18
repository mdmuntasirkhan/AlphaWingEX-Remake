#pragma once
#ifndef BOSSBULLETS_H
#define BOSSBULLETS_H

#include "Mesh.h"
#include "Shader.h"
#include <Matrix.h>
#include <Vector.h>
#include <vector>

using namespace MATH;

// Settings for a guided missile. It works like the player's missile in Bullet.cpp, just weaker.
struct MissileGuidance {
	float launchSpeed = 5.0f;      // fast straight burst right after it is fired
	float launchTime = 0.4f;       // seconds of straight flight before the steering starts
	float cruiseSpeed = 1.6f;      // slow hunting speed after the burst
	float decelTime = 1.4f;        // seconds it takes to slow from launch speed to cruise speed
	float terminalSpeed = 2.6f;    // speed it goes at when it is close to the player
	float terminalRange = 1.8f;    // how close counts as "close", it also stops snaking inside this range
	float turnRate = 120.0f;       // degrees per second it can turn toward the player, higher tracks tighter
	float maxLeadTime = 0.6f;      // it aims at where the player will be, but looks ahead at most this many seconds
	float weaveAngle = 0.5f;       // radians it snakes to each side while it is far away, 0 turns it off
	float weaveSpeed = 2.2f;       // how fast it snakes
	float lifetime = 30.0f;        // seconds until it blows up on its own
};

// One orb or missile fired by the boss
struct BossBullet {
	Vec3  pos{};
	Vec3  vel{};
	Vec3  color{};
	float radius = 0.15f;     // world units, used for the hit check
	float damage = 10.0f;     // damage to the player on a hit
	int   health = 0;         // above 0 means the player's lasers can shoot it down

	// Look
	float age = 0.0f;         // seconds since it was fired
	float growTime = 0.2f;    // seconds it takes to grow from small to full size

	// Guided missile. If guided is false it is a plain orb that flies straight.
	bool  guided = false;
	MissileGuidance guide;
	float size = 0.2f;        // how big the missile mesh is drawn (the player's missile is 0.3)
	float weavePhase = 0.0f;  // offsets the snaking, so missiles fired together are out of step

	// Ghost trail: where the bullet was a moment ago, newest first
	int   trailCount = 0;     // how many ghosts it draws, 0 means none (the most is kMaxTrail)
	static constexpr int kMaxTrail = 8;
	Vec3  trail[kMaxTrail]{};
	float trailAngle[kMaxTrail]{};   // the way the missile was facing at each ghost, in degrees
	int   trailUsed = 0;
	float trailDist = 0.0f;   // distance traveled since the last ghost was dropped
};

// Holds every bullet the boss has fired, moves them and draws them.
class BossBullets {
private:
	Mesh* orbMesh;
	Mesh* missileMesh;
	std::vector<BossBullet> bullets;

	static constexpr float kMeshRadius = 0.3f;      // radius of the orb OBJ, used to scale it to the bullet radius
	static constexpr float kGhostSpacing = 0.14f;   // distance between ghosts
	static constexpr float kGhostAlpha = 0.5f;      // how see-through the newest ghost is, older ones fade from here
	static constexpr float kStartScale = 0.25f;     // size a bullet starts at while it grows in

public:
	BossBullets();
	~BossBullets();

	bool OnCreate(const char* orbMeshFile, const char* missileMeshFile);
	void OnDestroy();
	void Update(float deltaTime, const Vec3& playerPos, const Vec3& playerVel);
	void Render(Shader* shader, const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const;

	// returns the new bullet so the caller can set its extra numbers right after
	BossBullet& Spawn(const Vec3& pos, const Vec3& vel, const Vec3& color, float radius);
	void RemoveAt(int index);
	void Clear() { bullets.clear(); }
	std::vector<BossBullet>& GetBullets() { return bullets; }
};

#endif // BOSSBULLETS_H