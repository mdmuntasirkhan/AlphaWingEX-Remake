#ifndef BOSSSPIDER_H
#define BOSSSPIDER_H

#include "Enemy.h"
#include "AnimatedMesh.h"
#include "BossBullets.h"
#include "GameConstants.h"

// The attacks the boss can pick from. The tables in BossSpider.cpp say which comes when.
enum class BossAttack { FAN, RING, SEEKER };

// The spider boss.
// It walks in slowly, then moves between a few set points while its head follows the player.
// At each stop it fires one attack. Which attack depends on the phase, and the phase
// changes when the boss loses health (100% to 60%, 60% to 30%, then under 30%).
class BossSpider : public Enemy {
private:
	enum class State { ENTERING, MOVING, PAUSING, DYING, DEAD };

	AnimatedMesh* creep;
	AnimatedMesh* walk;
	AnimatedMesh* skitter;
	AnimatedMesh* current;    // the clip that is playing right now
	BossBullets* bullets;    // borrowed from the scene, not owned here

	State state;
	Vec3  pos;
	float facingAngle;        // degrees around Z: 0 faces left, -90 faces up, 90 faces down
	float lean;               // extra angle while walking up or down
	int   waypointIndex;      // which point of the patrol path it is walking to
	float stateTimer;
	float entranceSpeed;      // world units per second while walking in
	float floorSpeed;         // how fast the arena floor scrolls left under the boss, so the feet can match it

	// Health
	float health;
	float healthTrail;        // the pale part of the bar, it follows the health down slowly
	float trailDelay;         // seconds the pale part waits after a hit before it starts to drain
	float hitFlash;           // counts down from kHitFlashDuration after a hit
	int   phase;              // 1, 2 or 3
	float dyingTimer;
	bool  deathRequested;     // true once when the death is over, the scene reads it with PopDeathRequest()

	// Attack state
	int   attackIndex;        // how many attacks it has started in this phase, picks the next one from the table
	BossAttack attack;        // the attack of the current stop
	int   volleysTotal;       // volleys planned for this stop
	int   volleysLeft;        // volleys still to fire during this stop
	float volleyTimer;        // seconds until the next volley
	float volleyInterval;     // seconds between volleys
	float warningTime;        // how long the glow lasts before the first volley
	float pauseTime;          // how long this stop lasts in total
	float glow;               // 0 to 1, the warning glow before it shoots
	float ringGap;            // angle the gap of the next ring is turned to, in radians

	// Tuning
	static constexpr float kScale = 0.7f;
	// ground speed of each clip at scale 1 (model units per second), measured from the animation
	static constexpr float kCreepFoot = 0.472f;
	static constexpr float kWalkFoot = 1.233f;
	static constexpr float kSkitterFoot = 2.337f;

	static constexpr float kHoldX = 7.5f;    // where it stops after coming in
	static constexpr float kEntranceTime = 8.0f;    // seconds it takes to walk in
	static constexpr float kPatrolSpeed = 1.4f;    // world units per second
	static constexpr float kAfterShot = 0.3f;    // seconds it stays at a stop after the last volley

	static constexpr float kTrackTurnSpeed = 80.0f;   // how fast the head follows the player, degrees per second
	static constexpr float kMaxTrackAngle = 70.0f;   // the head never turns more than this from facing left
	static constexpr float kLeanAngle = 15.0f;   // head leans toward the up or down direction, 0 turns it off

	static constexpr float kHeadOffset = 1.7f;    // distance from the body center to the head, where bullets come out
	static constexpr float kWarningGlow = 0.45f;   // how white the body gets at the end of the warning

	// Health and hit area
	static constexpr float kMaxHealth = 400.0f;
	static constexpr float kHitRadiusX = 1.5f;     // the hit area is an ellipse around the body
	static constexpr float kHitRadiusY = 1.0f;
	static constexpr float kHitFlashStrength = 0.6f;   // how white the body gets when it is hit
	static constexpr float kTrailWait = 0.5f;     // seconds the pale part of the bar waits before draining
	static constexpr float kTrailDrain = 80.0f;    // health per second the pale part drains
	static constexpr float kDyingTime = 2.0f;     // seconds from the last hit to the end of the boss

	// Fan shot
	static constexpr int   kFanVolleys = 3;       // volleys per stop
	static constexpr int   kFanHalfCount = 2;       // 2 means 5 bullets in a volley (2 each side plus the middle)
	static constexpr float kFanWarning = 0.5f;    // seconds of glow before the first volley
	static constexpr float kFanInterval = 0.35f;   // seconds between volleys
	static constexpr float kFanSpread = 0.17f;   // radians between bullets in a volley
	static constexpr float kFanSpeed = 4.5f;    // bullet speed, world units per second
	static constexpr float kBulletRadius = 0.17f;

	// Ring burst (the violet bullets). A circle of bullets with a gap, the gap turns a little every ring.
	// The bullets start slow and speed up, so the player has time to find the gap.
	static constexpr int   kRingVolleys = 3;       // rings per stop
	static constexpr float kRingWarning = 0.6f;
	static constexpr float kRingInterval = 0.7f;
	static constexpr int   kRingCount = 16;      // bullets in a full circle
	static constexpr int   kRingGapCount = 2;       // bullets left out to make the gap
	static constexpr float kRingGapTurn = 0.7f;     // radians the gap turns from one ring to the next
	static constexpr float kRingStartSpeed = 1.2f;
	static constexpr float kRingMaxSpeed = 4.2f;
	static constexpr float kRingAccel = 2.5f;     // speed gained per second
	static constexpr float kRingRadius = 0.19f;
	static constexpr float kRingDamage = 12.0f;

	// Guided missiles (the pink ones). The guidance numbers are in MissileGuidance in BossBullets.h.
	static constexpr int   kSeekerCount = 3;       // missiles in a salvo, one is fired per volley
	static constexpr float kSeekerWarning = 0.6f;    // seconds of glow before the first missile
	static constexpr float kSeekerInterval = 0.35f;
	static constexpr float kSeekerSpread = 0.5f;    // radians between the launch angles
	static constexpr float kSeekerRadius = 0.12f;   // hit size
	static constexpr float kSeekerSize = 0.18f;    // drawn size, the player's missile is 0.3
	static constexpr int   kSeekerHealth = 2;       // laser hits needed to destroy one
	static constexpr float kSeekerDamage = 15.0f;   // damage to the player
	static constexpr int   kSeekerTrail = 6;       // ghosts behind each missile

	// the animation speed that makes the feet match a movement speed on the ground
	static float MatchSpeed(float footSpeed, float groundSpeed) {
		return groundSpeed / (footSpeed * kScale);
	}

	void PlayClip(AnimatedMesh* clip, float playSpeed);
	void PlayOnFloor(AnimatedMesh* clip, float footSpeed, float moveX, float moveY);
	void TrackPlayer(float deltaTime, float playerX, float playerY);
	void UpdateHealthState(float deltaTime);
	void StartPause();
	Vec3 GetHeadPosition() const;
	void FireFan(float playerX, float playerY);
	void FireRing(float playerX, float playerY);
	void FireSeeker(float playerX, float playerY, int index);

	void UpdateEntering(float deltaTime);
	void UpdateMoving(float deltaTime);
	void UpdatePausing(float deltaTime, float playerX, float playerY);
	void UpdateDying(float deltaTime);

public:
	// damage the player's weapons do to the boss
	static constexpr float kLaserDamage = 1.0f;
	static constexpr float kMissileDamage = 5.0f;

	// the health fractions where the phases change, the health bar draws marks there
	static constexpr float kPhase2At = 0.6f;   // phase 2 starts when health is under 60%
	static constexpr float kPhase3At = 0.3f;   // phase 3 starts when health is under 30%

	BossSpider();
	~BossSpider();

	bool OnCreate(BossBullets* bullets_);
	void OnDestroy() override;
	void Update(float deltaTime, float playerX = 0.0f, float playerY = 0.0f) override;
	void Render(Shader* shader,
		const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const override;
	void Reset() override;

	void SetFloorSpeed(float speed) { floorSpeed = speed; }

	// Health. TakeDamage returns true when that hit killed the boss.
	bool TakeDamage(float amount);
	bool IsAlive() const { return state != State::DYING && state != State::DEAD; }
	bool IsHit(const Vec3& point, float extraRadius) const;   // is a point inside the hit area
	bool PopDeathRequest() { bool v = deathRequested; deathRequested = false; return v; }

	// For the health bar
	Vec3  GetPosition() const { return pos; }
	float GetHealthFraction() const { return health / kMaxHealth; }
	float GetHealthTrailFraction() const { return healthTrail / kMaxHealth; }
	bool  IsOnScreen() const { return state != State::DEAD && pos.x < GameConst::kSpawnX - 2.5f; }
	int   GetPhase() const { return phase; }
};

#endif // BOSSSPIDER_H
