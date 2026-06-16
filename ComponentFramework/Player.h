#pragma once
#ifndef PLAYER_H
#define PLAYER_H

#include "Mesh.h"
#include "Shader.h"
#include <Matrix.h>
#include <Vector.h>
#include <SDL3/SDL_events.h>

using namespace MATH;

union SDL_Event;

class Player {
private:
	Mesh* mesh;
	Matrix4 modelMatrix;

	Vec3 pos;
	float speed;

	float health;
	float maxHealth;
	int lives;

	// Attributes
	Vec3 velocity;
	float thrustPower;
	float friction;
	float maxSpeed;

	// Shield
	Mesh* shieldMesh;
	bool shieldActive;
	float shieldTimer;
	float shieldDuration;
	float shieldCooldown;
	float shieldCooldownTimer;
	bool shieldOnCooldown;

	// Shield crescent effect - three crescent-shaped glints, evenly spaced
	// 1/3 of a cycle apart, continuously slide down the shield's front curve
	// and wrap back to the top - reads as 3 chunks rotating top to bottom.
	float shieldSweepTimer;
	float shieldSweepPeriod;		 // seconds for one full top-to-bottom pass
	float shieldYRadius;			 // shield mesh's local Y radius (it spans -2.5..2.5)
	float shieldZRadius;			 // shield mesh's local Z radius (it spans -3.0..3.0) -
									 // used to trace each crescent along the front curve
	float shieldGlowRadius;		 // falloff radius of each crescent, in local-space units

	// Given a 0..1 phase through the sweep cycle, returns the point (on the
	// shield's front curve, in local space) that crescent should center on.
	Vec3 ComputeShieldGlowPoint(float phase) const;

public:
	Player();
	~Player();

	bool OnCreate(const char* meshFile);
	void OnDestroy();
	void Update(float deltaTime);
	void Render(Shader* shader,
		const Matrix4& projectionMatrix,
		const Matrix4& viewMatrix) const;
	void HandleEvents(const SDL_Event& sdlEvent);

	// Getters
	Vec3 GetPosition() const { return pos; }
	float GetHealth() const { return health; }
	int GetLives() const { return lives; }
	bool IsGameOver() const { return lives <= 0; }

	// Danage and Reset
	void TakeDamage(float amount);
	void Reset();

	// Shield
	void ActivateShield();
	bool IsShieldActive() const { return shieldActive; }
	bool IsShieldOnCooldown() const { return shieldOnCooldown; }
	float GetShieldCooldownPercent() const { return shieldCooldownTimer / shieldCooldown; }
};

#endif // !PLAYER_H
