#ifndef BOSSSPIDER_H
#define BOSSSPIDER_H

#include "Enemy.h"
#include "AnimatedMesh.h"
#include "BossBullets.h"

// The spider boss.
// It walks in slowly, then moves between a few set points (forward, backward, up and down)
// while its head always follows the player. At each stop it fires fan shots or seeker orbs.
class BossSpider : public Enemy {
private:
	enum class State { ENTERING, MOVING, PAUSING };

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

	// Attack state
	int   stopCount;          // how many stops it has made, used to alternate the attacks
	bool  seekerStop;         // true when this stop fires seekers, false when it fires the fan
	int   volleysTotal;       // volleys planned for this stop
	int   volleysLeft;        // volleys still to fire during this stop
	float volleyTimer;        // seconds until the next volley
	float warningTime;        // how long the glow lasts before the first volley
	float glow;               // 0 to 1, the pink warning glow before it shoots

	// Tuning
	static constexpr float kScale = 0.7f;
	// ground speed of each clip at scale 1 (model units per second), measured from the animation
	static constexpr float kCreepFoot = 0.472f;
	static constexpr float kWalkFoot = 1.233f;
	static constexpr float kSkitterFoot = 2.337f;

	static constexpr float kHoldX = 7.5f;    // where it stops after coming in
	static constexpr float kEntranceTime = 8.0f;    // seconds it takes to walk in
	static constexpr float kPatrolSpeed = 1.4f;    // world units per second
	static constexpr float kEndPause = 1.5f;    // seconds it stops at each point
	static constexpr float kPausePlaySpeed = 0.4f;    // slow leg animation while it stands

	static constexpr float kTrackTurnSpeed = 80.0f;   // how fast the head follows the player, degrees per second
	static constexpr float kMaxTrackAngle = 70.0f;   // the head never turns more than this from facing left
	static constexpr float kLeanAngle = 15.0f;   // head leans toward the up or down direction, 0 turns it off

	static constexpr float kHeadOffset = 1.7f;    // distance from the body center to the head, where bullets come out
	static constexpr float kWarningGlow = 0.45f;   // how white the body gets at the end of the warning

	// Fan shot
	static constexpr int   kFanVolleys = 3;       // volleys per stop
	static constexpr int   kFanHalfCount = 2;       // 2 means 5 bullets in a volley (2 each side plus the middle)
	static constexpr float kFanWarning = 0.5f;    // seconds of glow before the first volley
	static constexpr float kVolleyInterval = 0.35f;   // seconds between volleys
	static constexpr float kFanSpread = 0.17f;   // radians between bullets in a volley
	static constexpr float kFanSpeed = 4.5f;    // bullet speed, world units per second
	static constexpr float kBulletRadius = 0.17f;

	// Guided missiles (the pink ones). The guidance numbers are in MissileGuidance in BossBullets.h.
	static constexpr int   kSeekerCount = 3;       // missiles in a salvo, one is fired per volley
	static constexpr float kSeekerWarning = 0.6f;    // seconds of glow before the first missile
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
	void TrackPlayer(float deltaTime, float playerX, float playerY);
	void StartPause();
	Vec3 GetHeadPosition() const;
	void FireFan(float playerX, float playerY);
	void FireSeeker(float playerX, float playerY, int index);

	void UpdateEntering(float deltaTime);
	void UpdateMoving(float deltaTime);
	void UpdatePausing(float deltaTime, float playerX, float playerY);

public:
	BossSpider();
	~BossSpider();

	bool OnCreate(BossBullets* bullets_);
	void OnDestroy() override;
	void Update(float deltaTime, float playerX = 0.0f, float playerY = 0.0f) override;
	void Render(Shader* shader,
		const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const override;
	void Reset() override;
};

#endif // BOSSSPIDER_H