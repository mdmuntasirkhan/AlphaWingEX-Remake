#ifndef BOSSSPIDER_H
#define BOSSSPIDER_H

#include "Enemy.h"
#include "AnimatedMesh.h"

// The spider boss.
// This step: it walks in slowly, then moves between a few set points
// (forward, backward, up and down) while its head always follows the player.
// The attacks come in later steps.
class BossSpider : public Enemy {
private:
	enum class State { ENTERING, MOVING, PAUSING };

	AnimatedMesh* creep;
	AnimatedMesh* walk;
	AnimatedMesh* skitter;
	AnimatedMesh* current;    // the clip that is playing right now

	State state;
	Vec3  pos;
	float facingAngle;        // degrees around Z: 0 faces left, -90 faces up, 90 faces down
	float lean;               // extra angle while walking up or down
	int   waypointIndex;      // which point of the patrol path it is walking to
	float stateTimer;
	float entranceSpeed;      // world units per second while walking in

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

	// the animation speed that makes the feet match a movement speed on the ground
	static float MatchSpeed(float footSpeed, float groundSpeed) {
		return groundSpeed / (footSpeed * kScale);
	}

	void PlayClip(AnimatedMesh* clip, float playSpeed);
	void TrackPlayer(float deltaTime, float playerX, float playerY);
	void StartPause();

	void UpdateEntering(float deltaTime);
	void UpdateMoving(float deltaTime);
	void UpdatePausing(float deltaTime);

public:
	BossSpider();
	~BossSpider();

	bool OnCreate();
	void OnDestroy() override;
	void Update(float deltaTime, float playerX = 0.0f, float playerY = 0.0f) override;
	void Render(Shader* shader,
		const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const override;
	void Reset() override;
};

#endif // BOSSSPIDER_H