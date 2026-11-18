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

// The attack order of each phase. At every stop the boss takes the next one in its phase's row
// and goes back to the start of the row when it reaches the end.
// To change the fight, just change these rows. The dash and the spiral will be added here later.
static const int kAttacksPerPhase = 4;
static const BossAttack kAttackTable[3][kAttacksPerPhase] = {
	{ BossAttack::FAN,  BossAttack::RING,   BossAttack::FAN,    BossAttack::RING },     // phase 1
	{ BossAttack::FAN,  BossAttack::RING,   BossAttack::SEEKER, BossAttack::RING },     // phase 2
	{ BossAttack::RING, BossAttack::SEEKER, BossAttack::FAN,    BossAttack::SEEKER },   // phase 3
};

static const float kIdlePlaySpeed = 0.4f;   // slow leg shuffle while it stands still on a floor that does not move

// moves a value toward a target without going past it
static float MoveToward(float value, float target, float maxStep) {
	if (fabsf(target - value) <= maxStep) return target;
	return value + (target > value ? maxStep : -maxStep);
}

BossSpider::BossSpider() :
	creep{ nullptr }, walk{ nullptr }, skitter{ nullptr }, current{ nullptr },
	bullets{ nullptr },
	state{ State::ENTERING },
	pos{ GameConst::kSpawnX, 0.0f, GameConst::kWorldZ },
	facingAngle{ 0.0f }, lean{ 0.0f }, waypointIndex{ 0 },
	stateTimer{ 0.0f }, entranceSpeed{ 1.0f }, floorSpeed{ 0.0f },
	health{ kMaxHealth }, healthTrail{ kMaxHealth }, trailDelay{ 0.0f }, hitFlash{ 0.0f },
	phase{ 1 }, dyingTimer{ 0.0f }, deathRequested{ false },
	attackIndex{ 0 }, attack{ BossAttack::FAN }, volleysTotal{ 0 }, volleysLeft{ 0 },
	volleyTimer{ 0.0f }, volleyInterval{ kFanInterval }, warningTime{ kFanWarning },
	pauseTime{ 0.0f }, glow{ 0.0f }, ringGap{ 0.0f } {
}

BossSpider::~BossSpider() {}

bool BossSpider::OnCreate(BossBullets* bullets_) {
	bullets = bullets_;

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
	bullets = nullptr;
}

void BossSpider::Update(float deltaTime, float playerX, float playerY) {
	if (state == State::DEAD) return;

	lean = 0.0f;   // only the moving state sets it
	glow = 0.0f;   // only the warning before a shot sets it

	UpdateHealthState(deltaTime);

	switch (state) {
	case State::ENTERING: UpdateEntering(deltaTime);                    break;
	case State::MOVING:   UpdateMoving(deltaTime);                      break;
	case State::PAUSING:  UpdatePausing(deltaTime, playerX, playerY);   break;
	case State::DYING:    UpdateDying(deltaTime);                       break;
	default: break;
	}

	// a dying boss stops following the player
	if (state != State::DYING) TrackPlayer(deltaTime, playerX, playerY);
	current->Update(deltaTime);
}

void BossSpider::PlayClip(AnimatedMesh* clip, float playSpeed) {
	current = clip;
	current->SetPlaySpeed(playSpeed);
}

// Plays a clip so the feet match the floor. The floor scrolls left at floorSpeed,
// so the speed of the boss over the floor is its own movement plus floorSpeed to the right.
void BossSpider::PlayOnFloor(AnimatedMesh* clip, float footSpeed, float moveX, float moveY) {
	float overX = moveX + floorSpeed;
	float overY = moveY;
	float over = sqrtf(overX * overX + overY * overY);

	// standing still over the floor, so only a slow shuffle
	if (over < 0.05f) {
		PlayClip(clip, kIdlePlaySpeed);
		return;
	}

	// the head points toward -X at angle 0, so the way it faces is (-cos, -sin)
	// forward is 1 when it walks head first and -1 when it walks backward
	float rad = facingAngle * GameConst::kPi / 180.0f;
	float forward = (overX * -cosf(rad) + overY * -sinf(rad)) / over;

	// walking backward plays the legs in reverse so the feet match the floor
	float direction = (forward < -0.3f) ? -1.0f : 1.0f;
	PlayClip(clip, direction * MatchSpeed(footSpeed, over));
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

// the hit flash, the pale part of the health bar, and the phase
void BossSpider::UpdateHealthState(float deltaTime) {
	if (hitFlash > 0.0f) hitFlash -= deltaTime;

	// the pale part of the bar waits a moment after a hit, then drains down to the real health
	if (healthTrail > health) {
		if (trailDelay > 0.0f) {
			trailDelay -= deltaTime;
		}
		else {
			healthTrail -= kTrailDrain * deltaTime;
			if (healthTrail < health) healthTrail = health;
		}
	}

	// the phase comes from how much health is left
	float fraction = health / kMaxHealth;
	int newPhase = 1;
	if (fraction < kPhase3At) newPhase = 3;
	else if (fraction < kPhase2At) newPhase = 2;

	// a new phase starts its attack list from the beginning
	if (newPhase != phase) {
		phase = newPhase;
		attackIndex = 0;
	}
}

bool BossSpider::TakeDamage(float amount) {
	if (!IsAlive()) return false;

	health -= amount;
	hitFlash = GameConst::kHitFlashDuration;
	trailDelay = kTrailWait;
	if (health > 0.0f) return false;

	// that hit killed it. The bullets on the screen go away with it.
	health = 0.0f;
	state = State::DYING;
	dyingTimer = 0.0f;
	volleysLeft = 0;
	if (bullets) bullets->Clear();
	return true;
}

// the hit area is an ellipse around the body, plus the size of whatever is hitting it
bool BossSpider::IsHit(const Vec3& point, float extraRadius) const {
	if (!IsAlive()) return false;

	float dx = (point.x - pos.x) / (kHitRadiusX + extraRadius);
	float dy = (point.y - pos.y) / (kHitRadiusY + extraRadius);
	return dx * dx + dy * dy < 1.0f;
}

void BossSpider::StartPause() {
	stateTimer = 0.0f;
	state = State::PAUSING;

	// take the next attack from the table of the current phase
	attack = kAttackTable[phase - 1][attackIndex % kAttacksPerPhase];
	attackIndex++;

	switch (attack) {
	case BossAttack::FAN:
		volleysTotal = kFanVolleys;
		warningTime = kFanWarning;
		volleyInterval = kFanInterval;
		break;
	case BossAttack::RING:
		volleysTotal = kRingVolleys;
		warningTime = kRingWarning;
		volleyInterval = kRingInterval;
		break;
	case BossAttack::SEEKER:
		volleysTotal = kSeekerCount;
		warningTime = kSeekerWarning;
		volleyInterval = kSeekerInterval;
		break;
	}

	// every stop begins with the warning, then the volleys, then a short rest
	volleysLeft = volleysTotal;
	volleyTimer = warningTime;
	pauseTime = warningTime + (volleysTotal - 1) * volleyInterval + kAfterShot;
}

// the bullets come out of the head, which points the way the body faces
Vec3 BossSpider::GetHeadPosition() const {
	float rad = facingAngle * GameConst::kPi / 180.0f;
	return Vec3(pos.x - cosf(rad) * kHeadOffset, pos.y - sinf(rad) * kHeadOffset, pos.z);
}

void BossSpider::FireFan(float playerX, float playerY) {
	if (bullets == nullptr) return;

	Vec3 origin = GetHeadPosition();

	// aim at where the player is right now
	float aim = atan2f(playerY - origin.y, playerX - origin.x);
	for (int i = -kFanHalfCount; i <= kFanHalfCount; i++) {
		float a = aim + i * kFanSpread;
		Vec3 vel(cosf(a) * kFanSpeed, sinf(a) * kFanSpeed, 0.0f);
		bullets->Spawn(origin, vel, Vec3(1.0f, 0.46f, 0.11f), kBulletRadius);   // orange
	}
}

// A circle of bullets with a gap in it. It comes out of the middle of the body.
// The first ring of a stop has its gap facing the player, then every ring turns the gap a bit,
// so the player has to keep moving to stay in the gap.
void BossSpider::FireRing(float playerX, float playerY) {
	if (bullets == nullptr) return;

	if (volleysLeft == volleysTotal) {
		ringGap = atan2f(playerY - pos.y, playerX - pos.x);
	}

	float step = 2.0f * GameConst::kPi / kRingCount;

	// the first kRingGapCount spots are left empty, and they are centered on ringGap
	for (int i = kRingGapCount; i < kRingCount; i++) {
		float a = ringGap + (i - (kRingGapCount - 1) / 2.0f) * step;
		Vec3 vel(cosf(a) * kRingStartSpeed, sinf(a) * kRingStartSpeed, 0.0f);

		BossBullet& ring = bullets->Spawn(pos, vel, Vec3(0.6f, 0.4f, 1.0f), kRingRadius);   // violet
		ring.accel = kRingAccel;
		ring.maxSpeed = kRingMaxSpeed;
		ring.damage = kRingDamage;
	}

	ringGap += kRingGapTurn;
}

// fires one guided missile. The salvo is one missile per volley, with the launch angle spread out.
void BossSpider::FireSeeker(float playerX, float playerY, int index) {
	if (bullets == nullptr) return;

	Vec3 origin = GetHeadPosition();
	float aim = atan2f(playerY - origin.y, playerX - origin.x);

	// the first goes a bit up, the second straight at the player, the third a bit down
	float a = aim + (index - (kSeekerCount - 1) / 2.0f) * kSeekerSpread;

	BossBullet& missile = bullets->Spawn(origin, Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 0.25f, 0.6f), kSeekerRadius);   // pink
	missile.guided = true;
	missile.vel = Vec3(cosf(a) * missile.guide.launchSpeed, sinf(a) * missile.guide.launchSpeed, 0.0f);
	missile.size = kSeekerSize;
	missile.health = kSeekerHealth;
	missile.damage = kSeekerDamage;
	missile.trailCount = kSeekerTrail;
	missile.weavePhase = index * 2.1f;   // each missile snakes out of step with the others
}

void BossSpider::UpdateEntering(float deltaTime) {
	if (stateTimer == 0.0f) {
		// start just off the right edge of the screen
		pos.x = GameConst::kSpawnX;
		entranceSpeed = (pos.x - kHoldX) / kEntranceTime;
	}
	stateTimer += deltaTime;

	pos.x -= entranceSpeed * deltaTime;
	PlayOnFloor(creep, kCreepFoot, -entranceSpeed, 0.0f);

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

	// the legs follow the speed over the floor, and play in reverse when it walks backward
	PlayOnFloor(walk, kWalkFoot, dirX * kPatrolSpeed, dirY * kPatrolSpeed);

	// the head leans toward the up or down part of the movement
	lean = -dirY * kLeanAngle;
}

void BossSpider::UpdatePausing(float deltaTime, float playerX, float playerY) {
	stateTimer += deltaTime;

	// standing at a stop, the floor still slides by under it, so the legs keep stepping backward
	PlayOnFloor(creep, kCreepFoot, 0.0f, 0.0f);

	if (volleysLeft > 0) {
		volleyTimer -= deltaTime;

		// before the first volley the body glows more and more, so the player knows a shot is coming
		if (volleysLeft == volleysTotal && volleyTimer > 0.0f) {
			glow = kWarningGlow * (1.0f - volleyTimer / warningTime);
		}

		if (volleyTimer <= 0.0f) {
			switch (attack) {
			case BossAttack::FAN:    FireFan(playerX, playerY);                              break;
			case BossAttack::RING:   FireRing(playerX, playerY);                             break;
			case BossAttack::SEEKER: FireSeeker(playerX, playerY, volleysTotal - volleysLeft); break;
			}
			volleysLeft--;
			volleyTimer = volleyInterval;
		}
	}

	if (stateTimer >= pauseTime) {
		stateTimer = 0.0f;
		state = State::MOVING;
	}
}

void BossSpider::UpdateDying(float deltaTime) {
	dyingTimer += deltaTime;

	// the legs freeze and the body flickers white until it is gone
	current->SetPlaySpeed(0.0f);
	glow = 0.35f + 0.35f * sinf(dyingTimer * 30.0f);

	if (dyingTimer >= kDyingTime) {
		state = State::DEAD;
		deathRequested = true;
	}
}

void BossSpider::Render(Shader* shader,
	const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const {
	if (state == State::DEAD) return;

	glUniformMatrix4fv(shader->GetUniformID("projectionMatrix"), 1, GL_FALSE, projectionMatrix);
	glUniformMatrix4fv(shader->GetUniformID("viewMatrix"), 1, GL_FALSE, viewMatrix);

	// white, so the colors baked into the mesh show up as they are
	glUniform4f(shader->GetUniformID("color"), 1.0f, 1.0f, 1.0f, 1.0f);

	// the warning glow and the hit flash both use the flash uniform, the stronger one wins
	float flash = glow;
	float hit = hitFlash / GameConst::kHitFlashDuration * kHitFlashStrength;
	if (hit > flash) flash = hit;
	glUniform1f(shader->GetUniformID("flash"), flash);

	Matrix4 m = MMath::translate(pos) *
		MMath::rotate(facingAngle, Vec3(0.0f, 0.0f, 1.0f)) *
		MMath::scale(kScale, kScale, kScale);
	glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
	current->Render();

	// put it back to 0, a uniform stays set, so everything drawn after would glow too
	glUniform1f(shader->GetUniformID("flash"), 0.0f);
}

void BossSpider::Reset() {
	state = State::ENTERING;
	pos = Vec3(GameConst::kSpawnX, 0.0f, GameConst::kWorldZ);
	facingAngle = 0.0f;
	lean = 0.0f;
	waypointIndex = 0;
	stateTimer = 0.0f;
	health = kMaxHealth;
	healthTrail = kMaxHealth;
	trailDelay = 0.0f;
	hitFlash = 0.0f;
	phase = 1;
	dyingTimer = 0.0f;
	deathRequested = false;
	attackIndex = 0;
	volleysLeft = 0;
	volleyTimer = 0.0f;
	glow = 0.0f;
	current = creep;
	if (bullets) bullets->Clear();
}
