#include "Bullet.h"
#include <MMath.h>
#include <glew.h>
#include <iostream>
#include <cstdlib>

Bullet::Bullet() :  mesh{ nullptr },
					speed{ 10.0f },
					missileMesh{ nullptr },
					missileLaunchDuration{ 0.35f },
					missileSpeed{ 3.0f },
					missileNavigationGain{ 4.0f },
					missileMaxLateralAccel{ 40.0f },
					missileTerminalRange{ 3.0f },
					missileTerminalSpeedMultiplier{ 1.6f },
					missileCount{ 5 },
					maxMissiles{ 5 },
					missileReloadTimer{ 0.0f },
					missileReloadInterval{ 3.0f },
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
}

void Bullet::Spawn(Vec3 position) {
	if (fireCooldownTimer > 0.0f) return;
	fireCooldownTimer = fireCooldown;
	positions.push_back(position);
	float ySpread = ((rand() % 100) - 50) * 0.004f; // ±0.2 units/s subtle spread
	bulletYVelocities.push_back(ySpread);
}

void Bullet::SpawnHoming(Vec3 position, MissileTargetType targetType, int targetIndex) {
	if (missileCount <= 0) return;
	missileCount--;
	missilePositions.push_back(position);
	missileVelocities.push_back(Vec3(missileSpeed, 0.0f, 0.0f)); // launch straight forward
	missileTargetTypes.push_back(targetType);
	missileTargetIndices.push_back(targetIndex);
	missileLaunchTimers.push_back(0.0f);
}

bool Bullet::FindNearestTarget(const Vec3& fromPosition,
	const std::vector<Vec3>& asteroidPositions,
	const std::vector<Vec3>& bot01Positions,
	MissileTargetType& outType, int& outIndex) const {

	bool found = false;
	float bestDistSq = 0.0f;

	for (int a = 0; a < (int)asteroidPositions.size(); a++) {
		float dx = asteroidPositions[a].x - fromPosition.x;
		float dy = asteroidPositions[a].y - fromPosition.y;
		float distSq = dx * dx + dy * dy;
		if (!found || distSq < bestDistSq) {
			found = true;
			bestDistSq = distSq;
			outType = MissileTargetType::ASTEROID;
			outIndex = a;
		}
	}

	for (int b = 0; b < (int)bot01Positions.size(); b++) {
		float dx = bot01Positions[b].x - fromPosition.x;
		float dy = bot01Positions[b].y - fromPosition.y;
		float distSq = dx * dx + dy * dy;
		if (!found || distSq < bestDistSq) {
			found = true;
			bestDistSq = distSq;
			outType = MissileTargetType::BOT01;
			outIndex = b;
		}
	}

	return found;
}

void Bullet::Update(float deltaTime,
	const std::vector<Vec3>& asteroidPositions, const Vec3& asteroidVelocity,
	const std::vector<Vec3>& bot01Positions, const Vec3& bot01Velocity) {
	// Fire rate cooldown
	if (fireCooldownTimer > 0.0f) fireCooldownTimer -= deltaTime;

	// Missile reload — one missile every missileReloadInterval seconds
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
		if (positions[i].x > 15.0f) {
			positions.erase(positions.begin() + i);
			bulletYVelocities.erase(bulletYVelocities.begin() + i);
		}
	}

	// Homing missiles - proportional navigation guidance
	for (int i = 0; i < (int)missilePositions.size(); i++) {
		missileLaunchTimers[i] += deltaTime;

		// Re-acquire a target if the locked one is gone (destroyed mid-flight,
		// or never had one). Never store a raw pointer into Enemy's vectors -
		// they get resized/erased every frame and a stale pointer is undefined
		// behaviour (this was the cause of missiles randomly flying backward).
		int  tidx          = missileTargetIndices[i];
		bool lockedAsteroid = missileTargetTypes[i] == MissileTargetType::ASTEROID && tidx >= 0 && tidx < (int)asteroidPositions.size();
		bool lockedBot01    = missileTargetTypes[i] == MissileTargetType::BOT01    && tidx >= 0 && tidx < (int)bot01Positions.size();
		bool hasTarget      = lockedAsteroid || lockedBot01;

		if (!hasTarget) {
			MissileTargetType newType;
			int newIndex;
			if (FindNearestTarget(missilePositions[i], asteroidPositions, bot01Positions, newType, newIndex)) {
				missileTargetTypes[i] = newType;
				missileTargetIndices[i] = newIndex;
				hasTarget = true;
			}
		}

		const Vec3* targetPos = nullptr;
		Vec3 targetVel(0.0f, 0.0f, 0.0f);
		if (hasTarget && missileTargetTypes[i] == MissileTargetType::ASTEROID) {
			targetPos = &asteroidPositions[missileTargetIndices[i]];
			targetVel = asteroidVelocity;
		}
		else if (hasTarget && missileTargetTypes[i] == MissileTargetType::BOT01) {
			targetPos = &bot01Positions[missileTargetIndices[i]];
			targetVel = bot01Velocity;
		}

		// Fly straight forward for the launch grace period (or if there's no
		// target at all), THEN start guiding - this stops missiles from
		// immediately reversing direction the instant they're fired.
		if (missileLaunchTimers[i] >= missileLaunchDuration && targetPos != nullptr) {
			// --- Predictive lead targeting ---
			// First-order intercept: estimate where the target will be by the
			// time the missile (at its current speed) closes the distance to it.
			float currentSpeed = sqrt(missileVelocities[i].x * missileVelocities[i].x +
									   missileVelocities[i].y * missileVelocities[i].y);
			if (currentSpeed < 0.01f) currentSpeed = missileSpeed;

			float rawDx = targetPos->x - missilePositions[i].x;
			float rawDy = targetPos->y - missilePositions[i].y;
			float timeToIntercept = sqrt(rawDx * rawDx + rawDy * rawDy) / currentSpeed;

			float leadX = targetPos->x + targetVel.x * timeToIntercept;
			float leadY = targetPos->y + targetVel.y * timeToIntercept;

			// --- Proportional navigation toward the lead point ---
			float dx = leadX - missilePositions[i].x;
			float dy = leadY - missilePositions[i].y;
			float range = sqrt(dx * dx + dy * dy);

			if (range > 0.01f) {
				// Relative velocity of the (predicted) target w.r.t. the missile
				float relVelX = targetVel.x - missileVelocities[i].x;
				float relVelY = targetVel.y - missileVelocities[i].y;

				// Closing speed (positive = closing in) and line-of-sight
				// rotation rate (2D cross product over range squared) - these
				// two together are the actual proportional navigation law.
				float closingSpeed = -(dx * relVelX + dy * relVelY) / range;
				float losRate = (dx * relVelY - dy * relVelX) / (range * range);

				float lateralAccel = missileNavigationGain * closingSpeed * losRate;
				if (lateralAccel > missileMaxLateralAccel) lateralAccel = missileMaxLateralAccel;
				if (lateralAccel < -missileMaxLateralAccel) lateralAccel = -missileMaxLateralAccel;

				// Apply the commanded acceleration perpendicular to the line of sight
				float perpX = -(dy / range);
				float perpY = (dx / range);
				missileVelocities[i].x += perpX * lateralAccel * deltaTime;
				missileVelocities[i].y += perpY * lateralAccel * deltaTime;
			}

			// --- Terminal speed boost: floor it once close to the real target ---
			float realRange = sqrt(rawDx * rawDx + rawDy * rawDy);
			float desiredSpeed = (realRange < missileTerminalRange)
				? missileSpeed * missileTerminalSpeedMultiplier
				: missileSpeed;

			// Re-normalize to the desired speed, keeping the steered direction
			float speedNow = sqrt(missileVelocities[i].x * missileVelocities[i].x +
								   missileVelocities[i].y * missileVelocities[i].y);
			if (speedNow > 0.01f) {
				missileVelocities[i].x = missileVelocities[i].x / speedNow * desiredSpeed;
				missileVelocities[i].y = missileVelocities[i].y / speedNow * desiredSpeed;
			}
		}

		// Integrate position from velocity (during the launch grace period
		// velocity is still the straight-forward vector set at spawn time)
		missilePositions[i].x += missileVelocities[i].x * deltaTime;
		missilePositions[i].y += missileVelocities[i].y * deltaTime;
	}

	// Remove missiles off screen (any edge - PN guidance can curve them anywhere)
	for (int i = (int)missilePositions.size() - 1; i >= 0; i--) {
		if (missilePositions[i].x > 15.0f || missilePositions[i].x < -15.0f ||
			missilePositions[i].y > 10.0f || missilePositions[i].y < -10.0f) {
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

	// Draw Missiles
	for (int i = 0; i < (int)missilePositions.size(); i++) {
		Matrix4 missileMatrix = MMath::translate(missilePositions[i]) *
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
}