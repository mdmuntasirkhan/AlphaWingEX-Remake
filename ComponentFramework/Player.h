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
	Mesh* cockpitMesh;
	Mesh* attachmentMesh;
	Mesh* thrustMesh;
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
	bool  shieldActive;
	bool  shieldPaused;         // true = retracting (timer counts back, shield invisible)
	bool  shieldPenaltyQueued;  // true if retracted after ≥90% usage → penalty cooldown on full retract
	float shieldTimer;
	float shieldDuration;
	float shieldCooldown;
	float shieldCooldownTimer;
	bool  shieldOnCooldown;

	static constexpr float kPenaltyCooldown = 12.0f; // cooldown when shield expires or retracted after ≥90%

	float shieldSweepTimer;
	float shieldSweepPeriod;
	float shieldYRadius;
	float shieldZRadius;
	float shieldGlowRadius;

	float rollAngle;
	float rollVelocity;
	float rollStiffness;
	float rollDamping;
	float maxRollAngle;

	float thrustTimer;

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

	// Damage and Reset
	void TakeDamage(float amount);
	void ApplyImpulse(const Vec3& impulse) { velocity += impulse; }
	Vec3 GetVelocity() const { return velocity; }
	void Reset();

	// State restore (used by save/load system)
	void SetHealth(float h)           { health = (h > maxHealth ? maxHealth : h); }
	void SetLives(int l)              { lives  = l; }
	void SetPosition(float x, float y){ pos    = Vec3(x, y, -10.0f); velocity = Vec3(0,0,0); }

	// Shield
	void ActivateShield();
	bool  IsShieldActive()     const { return shieldActive && !shieldPaused; }
	bool  IsShieldRetracting() const { return shieldActive &&  shieldPaused; }
	bool  IsShieldPaused()     const { return shieldPaused; }
	bool  IsShieldOnCooldown() const { return shieldOnCooldown; }
	float GetShieldCooldownPercent()  const { return shieldCooldownTimer / shieldCooldown; }
	float GetShieldDurationPercent()  const { return shieldTimer / shieldDuration; }
	// 0=full charge, increases as shield drains; used by HUD and retract bar
	float GetShieldChargeFraction()   const { return 1.0f - (shieldTimer / shieldDuration); }
	// Elliptical collision half-axes: mesh bounds (±3.5 X, ±2.5 Y) × render scale 0.3
	float GetShieldRadiusX() const { return 1.05f; }
	float GetShieldRadiusY() const { return 0.75f; }
};

#endif // !PLAYER_H
