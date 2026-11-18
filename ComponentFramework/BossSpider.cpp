#include "BossSpider.h"
#include "GameConstants.h"
#include <MMath.h>
#include <cmath>

// The patrol path, x and y in world units. The boss goes through these in order and loops.
// Top back, top front (forward), bottom front, bottom back (backward), middle.
static const float kWaypoints[][2] = {
	{ 8.5f,  3.5f },
	{ 5.5f,  3.0f },
	{ 5.5f, -3.0f },
	{ 8.5f, -3.5f },
	{ 7.0f,  0.0f },
};
static const int kWaypointCount = 5;

// moves a value toward a target without going past it
static float MoveToward(float value, float target, float maxStep) {
	if (fabsf(target - value) <= maxStep) return target;
	return value + (target > value ? maxStep : -maxStep);
}

BossSpider::BossSpider() :
	creep{ nullptr }, walk{ nullptr }, skitter{ nullptr }, current{ nullptr },
	state{ State::ENTERING },
	pos{ GameConst::kSpawnX, 0.0f, GameConst::kWorldZ },
	facingAngle{ 0.0f }, lean{ 0.0f }, waypointIndex{ 0 },
	stateTimer{ 0.0f }, entranceSpeed{ 1.0f } {
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
	lean = 0.0f;   // only the moving state sets it

	switch (state) {
	case State::ENTERING: UpdateEntering(deltaTime); break;
	case State::MOVING:   UpdateMoving(deltaTime);   break;
	case State::PAUSING:  UpdatePausing(deltaTime);  break;
	}

	TrackPlayer(deltaTime, playerX, playerY);
	current->Update(deltaTime);
}

void BossSpider::PlayClip(AnimatedMesh* clip, float playSpeed) {
	current = clip;
	current->SetPlaySpeed(playSpeed);
}

void BossSpider::TrackPlayer(float deltaTime, float playerX, float playerY) {
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

void BossSpider::UpdateMoving(float deltaTime) {
	float targetX = kWaypoints[waypointIndex][0];
	float targetY = kWaypoints[waypointIndex][1];
	float dx = targetX - pos.x;
	float dy = targetY - pos.y;
	float dist = sqrtf(dx * dx + dy * dy);
	float step = kPatrolSpeed * deltaTime;

	// close enough to reach the point this frame, so stop there and go to the next one
	if (dist <= step) {
		pos.x = targetX;
		pos.y = targetY;
		waypointIndex = (waypointIndex + 1) % kWaypointCount;
		StartPause();
		return;
	}

	float dirX = dx / dist;
	float dirY = dy / dist;
	pos.x += dirX * step;
	pos.y += dirY * step;

	// forward means the way the head points. 1 is walking head first, -1 is walking backward
	float rad = facingAngle * GameConst::kPi / 180.0f;
	float forward = dirX * -cosf(rad) + dirY * -sinf(rad);

	// walking backward plays the legs in reverse so the feet match the ground
	float direction = (forward < -0.3f) ? -1.0f : 1.0f;
	PlayClip(walk, direction * MatchSpeed(kWalkFoot, kPatrolSpeed));

	// the head leans toward the up or down part of the movement
	lean = -dirY * kLeanAngle;
}

void BossSpider::UpdatePausing(float deltaTime) {
	stateTimer += deltaTime;
	PlayClip(creep, kPausePlaySpeed);

	if (stateTimer >= kEndPause) {
		stateTimer = 0.0f;
		state = State::MOVING;
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
	lean = 0.0f;
	waypointIndex = 0;
	stateTimer = 0.0f;
	current = creep;
}