#include "BossSpider.h"
#include "GameConstants.h"
#include <MMath.h>
#include <cmath>

// moves a value toward a target without going past it
static float MoveToward(float value, float target, float maxStep) {
	if (fabsf(target - value) <= maxStep) return target;
	return value + (target > value ? maxStep : -maxStep);
}

BossSpider::BossSpider() :
	creep{ nullptr }, walk{ nullptr }, skitter{ nullptr }, current{ nullptr },
	state{ State::ENTERING },
	pos{ GameConst::kSpawnX, 0.0f, GameConst::kWorldZ },
	facingAngle{ 0.0f }, patrolDir{ 1 }, stateTimer{ 0.0f }, entranceSpeed{ 1.0f } {
}

BossSpider::~BossSpider() {}

bool BossSpider::OnCreate() {
	// frame counts: the baked files plus one extra frame in each clip so the loop is smooth
	creep = new AnimatedMesh("meshes/Spider/Creep", 72);
	walk = new AnimatedMesh("meshes/Spider/Walk", 48);
	skitter = new AnimatedMesh("meshes/Spider/Skitter", 36);

	if (creep->OnCreate() == false || walk->OnCreate() == false || skitter->OnCreate() == false) {
		return false;
	}

	current = creep;
	return true;
}

void BossSpider::OnDestroy() {
	delete creep;
	delete walk;
	delete skitter;
	creep = walk = skitter = nullptr;
	current = nullptr;
}

void BossSpider::Update(float deltaTime, float playerX, float playerY) {
	// the head leans a little toward the way it is walking
	float lean = 0.0f;
	if (state == State::PATROLLING) lean = -patrolDir * kLeanAngle;
	TrackPlayer(deltaTime, playerX, playerY, lean);

	switch (state) {
	case State::ENTERING:   UpdateEntering(deltaTime);   break;
	case State::PATROLLING: UpdatePatrolling(deltaTime); break;
	case State::PAUSING:    UpdatePausing(deltaTime);    break;
	}
	current->Update(deltaTime);
}

void BossSpider::PlayClip(AnimatedMesh* clip, float playSpeed) {
	current = clip;
	current->SetPlaySpeed(playSpeed);
}

void BossSpider::TrackPlayer(float deltaTime, float playerX, float playerY, float lean) {
	float dx = playerX - pos.x;
	float dy = playerY - pos.y;

	// the head points toward -X at angle 0, so the angle to the player is atan2(-dy, -dx)
	float angle = atan2f(-dy, -dx) * 180.0f / GameConst::kPi;
	if (angle > kMaxTrackAngle) angle = kMaxTrackAngle;
	if (angle < -kMaxTrackAngle) angle = -kMaxTrackAngle;

	facingAngle = MoveToward(facingAngle, angle + lean, kTrackTurnSpeed * deltaTime);
}

void BossSpider::StartPause() {
	stateTimer = 0.0f;
	state = State::PAUSING;
}

void BossSpider::UpdateEntering(float deltaTime) {
	if (stateTimer == 0.0f) {
		// start just off the right edge of the screen
		pos.x = GameConst::kSpawnX;
		entranceSpeed = (pos.x - kHoldX) / kEntranceTime;
	}
	stateTimer += deltaTime;

	pos.x -= entranceSpeed * deltaTime;
	PlayClip(creep, MatchSpeed(kCreepFoot, entranceSpeed));

	if (pos.x <= kHoldX) {
		pos.x = kHoldX;
		StartPause();
	}
}

void BossSpider::UpdatePatrolling(float deltaTime) {
	pos.y += patrolDir * kPatrolSpeed * deltaTime;
	PlayClip(walk, MatchSpeed(kWalkFoot, kPatrolSpeed));

	float endY = patrolDir * kPatrolRange;
	bool arrived = (patrolDir == 1) ? (pos.y >= endY) : (pos.y <= endY);
	if (arrived) {
		pos.y = endY;
		patrolDir = -patrolDir;   // next time it goes the other way
		StartPause();
	}
}

void BossSpider::UpdatePausing(float deltaTime) {
	stateTimer += deltaTime;
	PlayClip(creep, kPausePlaySpeed);

	if (stateTimer >= kEndPause) {
		stateTimer = 0.0f;
		state = State::PATROLLING;
	}
}

void BossSpider::Render(Shader* shader,
	const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const {
	glUniformMatrix4fv(shader->GetUniformID("projectionMatrix"), 1, GL_FALSE, projectionMatrix);
	glUniformMatrix4fv(shader->GetUniformID("viewMatrix"), 1, GL_FALSE, viewMatrix);

	// white, so the colors baked into the mesh show up as they are
	glUniform4f(shader->GetUniformID("color"), 1.0f, 1.0f, 1.0f, 1.0f);
	glUniform1f(shader->GetUniformID("flash"), 0.0f);

	Matrix4 m = MMath::translate(pos) *
		MMath::rotate(facingAngle, Vec3(0.0f, 0.0f, 1.0f)) *
		MMath::scale(kScale, kScale, kScale);
	glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
	current->Render();
}

void BossSpider::Reset() {
	state = State::ENTERING;
	pos = Vec3(GameConst::kSpawnX, 0.0f, GameConst::kWorldZ);
	facingAngle = 0.0f;
	patrolDir = 1;
	stateTimer = 0.0f;
	current = creep;
}