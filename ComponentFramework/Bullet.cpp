#include "Bullet.h"
#include "GameConstants.h"
#include <MMath.h>
#include <glew.h>
#include <iostream>
#include <cstdlib>

Bullet::Bullet() :  mesh{ nullptr },
					speed{ 10.0f },
					missileMesh{ nullptr },
					missileLaunchDuration{ 0.4f },
					missileSpeed{ 2.5f },
					missileNavigationGain{ 4.5f },
					missileMaxLateralAccel{ 35.0f },
					missileTerminalRange{ 2.5f },
					missileLaunchSpeed{ 8.0f },
					missileCruiseSpeed{ 2.5f },
					missileTerminalSpeed{ 6.0f },
					missileDecelDuration{ 1.5f },
					missileMaxLifetime{ 25.0f },
					missileCount{ 3 },
					maxMissiles{ 3 },
					missileReloadTimer{ 0.0f },
					missileReloadInterval{ 3.0f },
					missileCooldown{ 1.0f },
					missileCooldownTimer{ 0.0f },
					fireCooldown{ 0.15f },
					fireCooldownTimer{ 0.0f }
{
	// Leave Empty
}

Bullet::~Bullet() {}

bool Bullet::OnCreate(const char* bulletMeshFile,
					  const char* missileMeshFile) {

	// Regular bullet mesh
	mesh = new Mesh(bulletMeshFile);
	if (mesh->OnCreate() == false) {
		std::cout << "Bullet mesh not found!\n";
		return false;
	}

	// Missile mesh
	missileMesh = new Mesh(missileMeshFile);
	if (missileMesh->OnCreate() == false) {
		std::cout << "Missile mesh not found!\n";
		return false;
	}
	return true;
}

void Bullet::OnDestroy() {
	if (mesh) {
		mesh->OnDestroy();
		delete mesh;
		mesh = nullptr;
	}
	if (missileMesh) {
		missileMesh->OnDestroy();
		delete missileMesh;
		missileMesh = nullptr;
	}
	positions.clear();
	bulletYVelocities.clear();
	missilePositions.clear();
	missileVelocities.clear();
	missileTargetTypes.clear();
	missileTargetIndices.clear();
	missileLaunchTimers.clear();
	missileLifetimers.clear();
}

void Bullet::Spawn(Vec3 position) {
	if (fireCooldownTimer > 0.0f) return;
	fireCooldownTimer = fireCooldown;
	positions.push_back(position);
	float ySpread = ((rand() % 100) - 50) * kBulletSpreadY; // ±0.2 units/s subtle spread
	bulletYVelocities.push_back(ySpread);
}

void Bullet::SpawnHoming(Vec3 position, MissileTargetType targetType, int targetIndex) {
	if (missileCount <= 0) return;
	if (missileCooldownTimer > 0.0f) return;
	missileCount--;
	missileCooldownTimer = missileCooldown;
	missilePositions.push_back(position);
	missileVelocities.push_back(Vec3(missileLaunchSpeed, 0.0f, 0.0f));
	missileTargetTypes.push_back(targetType);
	missileTargetIndices.push_back(targetIndex);
	missileLaunchTimers.push_back(0.0f);
	missileLifetimers.push_back(0.0f);
}

bool Bullet::FindNearestTarget(const Vec3& fromPosition,
	const std::vector<Vec3>& asteroidPositions,
	const std::vector<Vec3>& bot01Positions,
	const std::vector<Vec3>& bot02Positions,
	MissileTargetType& outType, int& outIndex) const {

	// Priority: Bot02 (mini-boss) > Bot01 > Asteroid.
	// Within each tier pick the nearest one; only fall through if the tier is empty.
	auto pickNearest = [&](const std::vector<Vec3>& pool, MissileTargetType type) -> bool {
		if (pool.empty()) return false;
		float bestDistSq = -1.0f;
		int   bestIdx    = 0;
		for (int i = 0; i < (int)pool.size(); i++) {
			float dx = pool[i].x - fromPosition.x;
			float dy = pool[i].y - fromPosition.y;
			float distSq = dx * dx + dy * dy;
			if (bestDistSq < 0.0f || distSq < bestDistSq) {
				bestDistSq = distSq;
				bestIdx    = i;
			}
		}
		outType  = type;
		outIndex = bestIdx;
		return true;
	};

	if (pickNearest(bot02Positions,    MissileTargetType::BOT02))    return true;
	if (pickNearest(bot01Positions,    MissileTargetType::BOT01))    return true;
	if (pickNearest(asteroidPositions, MissileTargetType::ASTEROID)) return true;
	return false;
}

void Bullet::Update(float deltaTime,
	const std::vector<Vec3>& asteroidPositions, const Vec3& asteroidVelocity,
	const std::vector<Vec3>& bot01Positions,   const Vec3& bot01Velocity,
	const std::vector<Vec3>& bot02Positions,   const Vec3& bot02Velocity) {
	// Fire rate cooldown
	if (fireCooldownTimer > 0.0f) fireCooldownTimer -= deltaTime;

	// Per-launch cooldown
	if (missileCooldownTimer > 0.0f) missileCooldownTimer -= deltaTime;

	// Missile reload — one slot every missileReloadInterval seconds
	if (missileCount < maxMissiles) {
		missileReloadTimer += deltaTime;
		if (missileReloadTimer >= missileReloadInterval) {
			missileReloadTimer = 0.0f;
			missileCount++;
		}
	}

	// Move regular bullets (X forward + slight Y spread)
	for (int i = 0; i < (int)positions.size(); i++) {
		positions[i].x += speed * deltaTime;
		positions[i].y += bulletYVelocities[i] * deltaTime;
	}

	// Remove regular bullets off the screen
	for (int i = (int)positions.size() - 1; i >= 0; i--) {
		if (positions[i].x > kBulletCullX) {
			positions.erase(positions.begin() + i);
			bulletYVelocities.erase(bulletYVelocities.begin() + i);
		}
	}

	// Homing missiles - three-phase proportional navigation guidance
	for (int i = 0; i < (int)missilePositions.size(); i++) {
		missileLaunchTimers[i] += deltaTime;
		missileLifetimers[i]   += deltaTime;

		// Target resolution: pick the nearest enemy of the locked type every frame.
		// This eliminates index staleness — when enemies die and vectors shrink,
		// a stored index silently points to the wrong enemy. Recomputing nearest
		// each frame is cheap and matches exactly how Bot02 guidance stays reliable.
		// Only re-acquire to a different type when the entire locked pool is gone.
		auto pickNearest = [&](const std::vector<Vec3>& pool) -> int {
			if (pool.empty()) return -1;
			float bestSq = -1.0f;
			int   best   = 0;
			Vec3  mp     = missilePositions[i];
			for (int k = 0; k < (int)pool.size(); k++) {
				float dx = pool[k].x - mp.x;
				float dy = pool[k].y - mp.y;
				float sq = dx*dx + dy*dy;
				if (bestSq < 0.0f || sq < bestSq) { bestSq = sq; best = k; }
			}
			return best;
		};

		bool hasTarget = false;
		int  freshIdx  = -1;
		switch (missileTargetTypes[i]) {
			case MissileTargetType::BOT02:
				freshIdx = pickNearest(bot02Positions);   break;
			case MissileTargetType::BOT01:
				freshIdx = pickNearest(bot01Positions);   break;
			case MissileTargetType::ASTEROID:
				freshIdx = pickNearest(asteroidPositions); break;
			default: break;
		}

		if (freshIdx >= 0) {
			missileTargetIndices[i] = freshIdx;
			hasTarget = true;
		} else {
			// Entire locked type is gone — re-acquire to highest available tier
			MissileTargetType newType;
			int newIndex;
			if (FindNearestTarget(missilePositions[i], asteroidPositions, bot01Positions, bot02Positions, newType, newIndex)) {
				missileTargetTypes[i]   = newType;
				missileTargetIndices[i] = newIndex;
				hasTarget = true;
			}
		}

		const Vec3* targetPos = nullptr;
		Vec3 targetVel(0.0f, 0.0f, 0.0f);
		float rawDx = 0.0f, rawDy = 0.0f;
		if (hasTarget && missileTargetTypes[i] == MissileTargetType::ASTEROID) {
			targetPos = &asteroidPositions[missileTargetIndices[i]];
			targetVel = asteroidVelocity;
		}
		else if (hasTarget && missileTargetTypes[i] == MissileTargetType::BOT01) {
			targetPos = &bot01Positions[missileTargetIndices[i]];
			targetVel = bot01Velocity;
		}
		else if (hasTarget && missileTargetTypes[i] == MissileTargetType::BOT02) {
			targetPos = &bot02Positions[missileTargetIndices[i]];
			targetVel = bot02Velocity;
		}

		if (targetPos) {
			rawDx = targetPos->x - missilePositions[i].x;
			rawDy = targetPos->y - missilePositions[i].y;
		}

		// Fly straight during the launch burst; PN guidance starts after it ends.
		// Low cruise speed gives the PN law plenty of time to steer — the missile
		// will curve back naturally if it overshoots, producing the near-miss feel.
		if (missileLaunchTimers[i] >= missileLaunchDuration && targetPos != nullptr) {
			// --- Predictive lead targeting ---
			float currentSpeed = sqrtf(missileVelocities[i].x * missileVelocities[i].x +
									    missileVelocities[i].y * missileVelocities[i].y);
			if (currentSpeed < 0.01f) currentSpeed = missileCruiseSpeed;

			float timeToIntercept = sqrtf(rawDx * rawDx + rawDy * rawDy) / currentSpeed;
			float leadX = targetPos->x + targetVel.x * timeToIntercept;
			float leadY = targetPos->y + targetVel.y * timeToIntercept;

			// --- Proportional navigation toward the lead point ---
			float dx    = leadX - missilePositions[i].x;
			float dy    = leadY - missilePositions[i].y;
			float range = sqrtf(dx * dx + dy * dy);

			if (range > 0.01f) {
				float relVelX      = targetVel.x - missileVelocities[i].x;
				float relVelY      = targetVel.y - missileVelocities[i].y;
				float closingSpeed = -(dx * relVelX + dy * relVelY) / range;
				float losRate      = (dx * relVelY - dy * relVelX) / (range * range);

				float lateralAccel = missileNavigationGain * closingSpeed * losRate;
				if (lateralAccel >  missileMaxLateralAccel) lateralAccel =  missileMaxLateralAccel;
				if (lateralAccel < -missileMaxLateralAccel) lateralAccel = -missileMaxLateralAccel;

				float perpX = -(dy / range);
				float perpY =  (dx / range);
				missileVelocities[i].x += perpX * lateralAccel * deltaTime;
				missileVelocities[i].y += perpY * lateralAccel * deltaTime;
			}
		}

		// --- Three-phase speed profile ---
		// Phase 1 (launch burst): fast, straight, exciting.
		// Phase 2 (decel/cruise): slows to hunting pace so guidance can correct any overshoot.
		// Phase 3 (terminal): accelerates for a decisive, unavoidable hit.
		float t = missileLaunchTimers[i];
		float desiredSpeed;
		if (t < missileLaunchDuration) {
			desiredSpeed = missileLaunchSpeed;
		} else {
			float cruiseT  = t - missileLaunchDuration;
			float lerpFrac = (cruiseT < missileDecelDuration) ? (cruiseT / missileDecelDuration) : 1.0f;
			desiredSpeed   = missileLaunchSpeed + lerpFrac * (missileCruiseSpeed - missileLaunchSpeed);
		}
		float realRange = sqrtf(rawDx * rawDx + rawDy * rawDy);
		if (hasTarget && realRange < missileTerminalRange) {
			desiredSpeed = missileTerminalSpeed;
		}

		float speedNow = sqrtf(missileVelocities[i].x * missileVelocities[i].x +
							    missileVelocities[i].y * missileVelocities[i].y);
		if (speedNow > 0.01f) {
			missileVelocities[i].x = missileVelocities[i].x / speedNow * desiredSpeed;
			missileVelocities[i].y = missileVelocities[i].y / speedNow * desiredSpeed;
		}

		missilePositions[i].x += missileVelocities[i].x * deltaTime;
		missilePositions[i].y += missileVelocities[i].y * deltaTime;
	}

	// Lifetime-only cull — no position bounds. Missiles that overshoot are pulled
	// back by PN guidance at cruise speed, producing the near-miss-then-return feel.
	for (int i = (int)missilePositions.size() - 1; i >= 0; i--) {
		if (missileLifetimers[i] > missileMaxLifetime) {
			RemoveMissileAt(i);
		}
	}

}

void Bullet::Render(Shader* shader,
					const Matrix4& projectionMatrix,
					const Matrix4& viewMatrix) const {
	glUniformMatrix4fv(shader->GetUniformID("projectionMatrix"),
		1, GL_FALSE, projectionMatrix);
	glUniformMatrix4fv(shader->GetUniformID("viewMatrix"),
		1, GL_FALSE, viewMatrix);

	// Draw regular bullets
	for (int i = 0; i < (int)positions.size(); i++) {
		Matrix4 bulletMatrix = MMath::translate(positions[i]) *
			MMath::scale(0.2f, 0.2f, 0.2f);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"),
			1, GL_FALSE, bulletMatrix);
		mesh->Render();
	}

	// Draw Missiles — rotate to face velocity direction
	for (int i = 0; i < (int)missilePositions.size(); i++) {
		float vx = missileVelocities[i].x;
		float vy = missileVelocities[i].y;
		float angleDeg = (vx * vx + vy * vy > 0.0001f)
			? atan2f(vy, vx) * (180.0f / 3.14159265f)
			: 0.0f;
		Matrix4 missileMatrix = MMath::translate(missilePositions[i]) *
								MMath::rotate(angleDeg, Vec3(0.0f, 0.0f, 1.0f)) *
								MMath::scale(0.3f, 0.3f, 0.3f);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"),
			1, GL_FALSE, missileMatrix);
		missileMesh->Render();
	}
}

void Bullet::RemoveAt(int index) {
	positions.erase(positions.begin() + index);
	bulletYVelocities.erase(bulletYVelocities.begin() + index);
}

void Bullet::RemoveMissileAt(int index) {
	missilePositions.erase(missilePositions.begin() + index);
	missileVelocities.erase(missileVelocities.begin() + index);
	missileTargetTypes.erase(missileTargetTypes.begin() + index);
	missileTargetIndices.erase(missileTargetIndices.begin() + index);
	missileLaunchTimers.erase(missileLaunchTimers.begin() + index);
	missileLifetimers.erase(missileLifetimers.begin() + index);
}