#include "BossBullets.h"
#include "GameConstants.h"
#include <MMath.h>
#include <cmath>
#include <iostream>

BossBullets::BossBullets() : orbMesh{ nullptr }, missileMesh{ nullptr } {}

BossBullets::~BossBullets() {}

bool BossBullets::OnCreate(const char* orbMeshFile, const char* missileMeshFile) {
	orbMesh = new Mesh(orbMeshFile);
	if (orbMesh->OnCreate() == false) {
		std::cout << "Boss orb mesh not found!\n";
		return false;
	}

	missileMesh = new Mesh(missileMeshFile);
	if (missileMesh->OnCreate() == false) {
		std::cout << "Boss missile mesh not found!\n";
		return false;
	}
	return true;
}

void BossBullets::OnDestroy() {
	if (orbMesh) {
		orbMesh->OnDestroy();
		delete orbMesh;
		orbMesh = nullptr;
	}
	if (missileMesh) {
		missileMesh->OnDestroy();
		delete missileMesh;
		missileMesh = nullptr;
	}
	bullets.clear();
}

BossBullet& BossBullets::Spawn(const Vec3& pos, const Vec3& vel, const Vec3& color, float radius) {
	BossBullet b;
	b.pos = pos;
	b.vel = vel;
	b.color = color;
	b.radius = radius;
	bullets.push_back(b);
	return bullets.back();
}

void BossBullets::RemoveAt(int index) {
	bullets.erase(bullets.begin() + index);
}

void BossBullets::Update(float deltaTime, const Vec3& playerPos, const Vec3& playerVel) {
	// go backward so erasing a bullet does not skip the next one
	for (int i = (int)bullets.size() - 1; i >= 0; i--) {
		BossBullet& b = bullets[i];
		b.age += deltaTime;

		// a missile that was bounced back by the shield stops steering and just flies straight
		if (b.guided && !b.reflected) {
			const MissileGuidance& g = b.guide;

			// a missile that lives too long blows up on its own
			if (b.age > g.lifetime) {
				bullets.erase(bullets.begin() + i);
				continue;
			}

			float speed = sqrtf(b.vel.x * b.vel.x + b.vel.y * b.vel.y);
			float rawDx = playerPos.x - b.pos.x;
			float rawDy = playerPos.y - b.pos.y;
			float realRange = sqrtf(rawDx * rawDx + rawDy * rawDy);

			// After the launch burst it steers toward the player.
			if (b.age >= g.launchTime) {
				if (speed < 0.01f) speed = g.cruiseSpeed;

				// aim at where the player will be, but only look a little way ahead
				float lookAhead = realRange / speed;
				if (lookAhead > g.maxLeadTime) lookAhead = g.maxLeadTime;
				float leadX = playerPos.x + playerVel.x * lookAhead;
				float leadY = playerPos.y + playerVel.y * lookAhead;

				// the angle it flies at now, and the angle it needs to fly at
				float heading = atan2f(b.vel.y, b.vel.x);
				float want = atan2f(leadY - b.pos.y, leadX - b.pos.x);

				// far away it snakes a little to each side, so the path roams instead of a straight chase
				// close to the player it stops snaking and goes straight for the hit
				if (realRange > g.terminalRange) {
					want += sinf(b.age * g.weaveSpeed + b.weavePhase) * g.weaveAngle;
				}

				float diff = want - heading;

				// keep the difference between -180 and 180 degrees so it turns the short way round
				while (diff > GameConst::kPi) diff -= 2.0f * GameConst::kPi;
				while (diff < -GameConst::kPi) diff += 2.0f * GameConst::kPi;

				// it can only turn a limited angle each frame
				float maxTurn = g.turnRate * GameConst::kPi / 180.0f * deltaTime;
				if (diff > maxTurn) diff = maxTurn;
				if (diff < -maxTurn) diff = -maxTurn;
				heading += diff;

				b.vel.x = cosf(heading) * speed;
				b.vel.y = sinf(heading) * speed;
			}

			// The speed it should have now: burst, then slowing down, then a sprint when close.
			float desiredSpeed;
			if (b.age < g.launchTime) {
				desiredSpeed = g.launchSpeed;
			}
			else {
				float cruiseT = b.age - g.launchTime;
				float frac = (cruiseT < g.decelTime) ? (cruiseT / g.decelTime) : 1.0f;
				desiredSpeed = g.launchSpeed + frac * (g.cruiseSpeed - g.launchSpeed);
			}
			if (realRange < g.terminalRange) desiredSpeed = g.terminalSpeed;

			// keep the direction the steering gave it, but set the length to the speed we want
			float speedNow = sqrtf(b.vel.x * b.vel.x + b.vel.y * b.vel.y);
			if (speedNow > 0.01f) {
				b.vel.x = b.vel.x / speedNow * desiredSpeed;
				b.vel.y = b.vel.y / speedNow * desiredSpeed;
			}
		}

		// a bullet with accel starts slow and speeds up until it reaches maxSpeed
		if (b.accel > 0.0f) {
			float s = sqrtf(b.vel.x * b.vel.x + b.vel.y * b.vel.y);
			if (s > 0.001f && s < b.maxSpeed) {
				float newSpeed = s + b.accel * deltaTime;
				if (newSpeed > b.maxSpeed) newSpeed = b.maxSpeed;
				b.vel.x = b.vel.x / s * newSpeed;
				b.vel.y = b.vel.y / s * newSpeed;
			}
		}

		b.pos += b.vel * deltaTime;

		// drop a ghost every kGhostSpacing of distance, so the spacing stays even at any speed
		if (b.trailCount > 0) {
			b.trailDist += sqrtf(b.vel.x * b.vel.x + b.vel.y * b.vel.y) * deltaTime;
			if (b.trailDist >= kGhostSpacing) {
				b.trailDist -= kGhostSpacing;
				for (int t = BossBullet::kMaxTrail - 1; t > 0; t--) {
					b.trail[t] = b.trail[t - 1];
					b.trailAngle[t] = b.trailAngle[t - 1];
				}
				b.trail[0] = b.pos;
				b.trailAngle[0] = atan2f(b.vel.y, b.vel.x) * 180.0f / GameConst::kPi;
				if (b.trailUsed < b.trailCount) b.trailUsed++;
			}
		}

		// remove bullets that left the screen. Missiles are allowed to go much farther out, since they come back.
		float pad = (b.guided && !b.reflected) ? 20.0f : 0.0f;
		if (b.pos.x < GameConst::kCullX - pad || b.pos.x > GameConst::kSpawnX + pad ||
			b.pos.y < -10.0f - pad || b.pos.y > 10.0f + pad)
		{
			bullets.erase(bullets.begin() + i);
		}
	}
}

void BossBullets::Render(Shader* shader,
	const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const {
	if (bullets.empty()) return;

	glUniformMatrix4fv(shader->GetUniformID("projectionMatrix"), 1, GL_FALSE, projectionMatrix);
	glUniformMatrix4fv(shader->GetUniformID("viewMatrix"), 1, GL_FALSE, viewMatrix);
	glUniform1f(shader->GetUniformID("flash"), 0.0f);
	glUniform1f(shader->GetUniformID("emissive"), 0.0f);

	// Solid part first: the orbs and the missiles
	for (const BossBullet& b : bullets) {
		// grow-in: 0 right after it is fired, 1 once growTime has passed
		float grow = (b.growTime > 0.0f) ? b.age / b.growTime : 1.0f;
		if (grow > 1.0f) grow = 1.0f;
		grow = 1.0f - (1.0f - grow) * (1.0f - grow);   // ease out, fast at first then it settles
		float scaleNow = kStartScale + (1.0f - kStartScale) * grow;

		glUniform4f(shader->GetUniformID("color"), b.color.x, b.color.y, b.color.z, 1.0f);

		if (b.guided) {
			// turn the missile to face the way it flies, like the player's missile
			float angle = atan2f(b.vel.y, b.vel.x) * 180.0f / GameConst::kPi;
			float s = b.size * scaleNow;
			Matrix4 model = MMath::translate(b.pos) *
				MMath::rotate(angle, Vec3(0.0f, 0.0f, 1.0f)) *
				MMath::scale(s, s, s);
			glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, model);
			missileMesh->Render();
		}
		else {
			float s = b.radius / kMeshRadius * scaleNow;
			Matrix4 model = MMath::translate(b.pos) * MMath::scale(s, s, s);
			glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, model);
			orbMesh->Render();
		}
	}

	// Ghosts: see-through copies left behind, each one fainter than the one before.
	// Blending is on and depth writing is off so they never cover the solid things.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	for (const BossBullet& b : bullets) {
		for (int t = 0; t < b.trailUsed; t++) {
			float alpha = kGhostAlpha * (1.0f - (float)t / (float)b.trailCount);
			glUniform4f(shader->GetUniformID("color"), b.color.x, b.color.y, b.color.z, alpha);

			if (b.guided) {
				float s = b.size * (1.0f - 0.06f * t);   // each ghost is a little smaller too
				Matrix4 model = MMath::translate(b.trail[t]) *
					MMath::rotate(b.trailAngle[t], Vec3(0.0f, 0.0f, 1.0f)) *
					MMath::scale(s, s, s);
				glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, model);
				missileMesh->Render();
			}
			else {
				float s = b.radius / kMeshRadius * (1.0f - 0.06f * t);
				Matrix4 model = MMath::translate(b.trail[t]) * MMath::scale(s, s, s);
				glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, model);
				orbMesh->Render();
			}
		}
	}
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}