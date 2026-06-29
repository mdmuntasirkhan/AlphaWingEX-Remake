#ifndef PLAYER_H
#define PLAYER_H

#include "Mesh.h"
#include "Shader.h"
#include "Sound.h"
#include <Matrix.h>
#include <Vector.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_audio.h>

using namespace MATH;

union SDL_Event;

class Player {
private:
	// Meshes
	Mesh* mesh;
	Mesh* cockpitMesh;
	Mesh* attachmentMesh;
	Mesh* thrustMesh;
	Matrix4 modelMatrix;

	// Position and Movement attributes
	Vec3 pos;
	float speed;
	Vec3 velocity;
	float thrustPower;
	float friction;
	float maxSpeed;

	// Health
	float health;
	float maxHealth;
	int lives;

	// Shield
	Mesh* shieldMesh;
	bool  shieldActive;
	float shieldTimer;       // 0 = full charge; counts up while active, down while recharging
	float shieldDuration;    // seconds to fully deplete from full charge
	float shieldRechargeRate; // multiplier on recharge speed; locked in when shield turns off

	//  < 80% used → kBaseRechargeRate  (fast)
	// ≥ 80% used → kRecharge80Rate    (medium penalty)
	// ≥ 90% or expired → kRecharge90Rate (heavy penalty)

	static constexpr float kBaseRechargeRate  = 1.0f;
	static constexpr float kRecharge80Rate    = 0.5f;
	static constexpr float kRecharge90Rate    = 0.15f;

	float shieldSweepTimer;
	float shieldSweepPeriod;
	float shieldYRadius;
	float shieldZRadius;
	float shieldGlowRadius;

	// Shield audio
	SDL_AudioStream* sfxStream;
	Sound* sfxShieldPhase01;
	Sound* sfxShieldPhase02;
	Sound* sfxShieldRecharged;
	Sound* sfxShieldDrain;
	float  prevShieldCharge;
	bool   prevShieldRecharging;
	float  shieldPhase01Cooldown;
	float  shieldPhase02Cooldown;
	float  shieldRechargedCooldown;

	static constexpr float kShieldPhaseCooldown     = 1.0f;
	static constexpr float kShieldRechargedCooldown = 2.0f;

	// Roll animation
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
	void SetSFXStream(SDL_AudioStream* stream) { sfxStream = stream; }
	void  ActivateShield();
	bool  IsShieldActive()     const { return shieldActive; }
	bool  IsShieldRecharging() const { return !shieldActive && shieldTimer > 0.0f; }
	float GetShieldChargeFraction()  const { return 1.0f - (shieldTimer / shieldDuration); }
	// Elliptical collision half axes: mesh bounds (±3.5 X, ±2.5 Y) × render scale 0.3
	float GetShieldRadiusX() const { return 1.05f; }
	float GetShieldRadiusY() const { return 0.75f; }
};

#endif // PLAYER_H
