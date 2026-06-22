#include "Bot01.h"
#include <MMath.h>
#include <glew.h>
#include <cstdlib>
#include <cmath>
#include <iostream>

// Wave cycle table — STANDARD (5-bot) → PINCER (top+bottom pair) → SHIELDED (missile-blocker)
static constexpr Bot01WaveDesc kWaves[] = {
    { Bot01WaveType::STANDARD, 5, 10.0f },
    { Bot01WaveType::PINCER,   2,  0.0f },
    { Bot01WaveType::SHIELDED, 1,  0.0f },
};
static constexpr int kWaveCount = 3;

Bot01::Bot01() :
	bot01Mesh{ nullptr },
	bot01ThrustMesh{ nullptr },
	fragmentMesh{ nullptr },
	shieldMesh{ nullptr },
	thrustTimer{ 0.0f },
	bot01SteerForce{ 10.0f },
	bot01YDamping{ 1.5f },
	bot01YMaxSpeed{ 8.0f },
	bot01Speed{ 1.2f },
	bot01SpawnTimer{ 0.0f },
	bot01SpawnInterval{ 10.0f },
	totalTime{ 0.0f },
	waveSize{ 0 },
	waveSpawned{ 0 },
	currentWaveIndex{ 0 },
	currentWaveType{ Bot01WaveType::STANDARD },
	spawningEnabled{ true }
{
}

Bot01::~Bot01() {}

bool Bot01::OnCreate(const char* meshFile, const char* fragmentMeshFile) {
	bot01Mesh = new Mesh(meshFile);
	if (bot01Mesh->OnCreate() == false) {
		std::cout << "Bot01 mesh not found!\n";
		return false;
	}

	bot01ThrustMesh = new Mesh("meshes/Temp_AlphaWing_Enemy_Bot01_Thrust.obj");
	if (bot01ThrustMesh->OnCreate() == false) {
		std::cout << "Bot01 thrust mesh not found!\n";
		return false;
	}

	fragmentMesh = new Mesh(fragmentMeshFile);
	if (fragmentMesh->OnCreate() == false) {
		std::cout << "Bot01 fragment mesh not found!\n";
		return false;
	}

	shieldMesh = new Mesh("meshes/Temp_AlphaWing_Shield.obj");
	if (shieldMesh->OnCreate() == false) {
		std::cout << "Bot01 shield mesh not found!\n";
		return false;
	}

	return true;
}

void Bot01::OnDestroy() {
	if (bot01Mesh) {
		bot01Mesh->OnDestroy();
		delete bot01Mesh;
		bot01Mesh = nullptr;
	}
	if (bot01ThrustMesh) {
		bot01ThrustMesh->OnDestroy();
		delete bot01ThrustMesh;
		bot01ThrustMesh = nullptr;
	}
	if (fragmentMesh) {
		fragmentMesh->OnDestroy();
		delete fragmentMesh;
		fragmentMesh = nullptr;
	}
	if (shieldMesh) {
		shieldMesh->OnDestroy();
		delete shieldMesh;
		shieldMesh = nullptr;
	}
	bot01Positions.clear();
	bot01XVelocities.clear();
	bot01YVelocities.clear();
	bot01XKnockbackVels.clear();
	bot01HP.clear();
	bot01HitTimers.clear();
	bot01HasShield.clear();
	bot01ShieldActive.clear();
	bot01ShieldOnCooldown.clear();
	bot01ShieldTimer.clear();
	bot01ShieldCooldownTimer.clear();
	debris.clear();
}

void Bot01::SpawnBot01(float x, float y, bool hasShield) {
	bot01Positions.push_back(Vec3(x, y, -10.0f));
	bot01XVelocities.push_back(0.0f);
	bot01YVelocities.push_back(0.0f);
	bot01XKnockbackVels.push_back(0.0f);
	bot01HP.push_back(10);
	bot01HitTimers.push_back(0.0f);
	bot01HasShield.push_back(hasShield);
	bot01ShieldActive.push_back(false);
	bot01ShieldOnCooldown.push_back(false);
	bot01ShieldTimer.push_back(0.0f);
	bot01ShieldCooldownTimer.push_back(0.0f);
}

void Bot01::Update(float deltaTime, float playerX, float playerY) {
	totalTime   += deltaTime;
	thrustTimer += deltaTime;

	if (spawningEnabled && waveSpawned < waveSize) {
		if (currentWaveType == Bot01WaveType::PINCER) {
			// Spawn top and bottom simultaneously on first tick of the wave
			if (waveSpawned == 0) {
				SpawnBot01(15.0f,  4.5f, false);
				SpawnBot01(15.0f, -4.5f, false);
				waveSpawned = 2;
			}
		} else if (currentWaveType == Bot01WaveType::SHIELDED) {
			if (waveSpawned == 0) {
				float spawnY = -3.5f + (rand() % 701) / 100.0f;
				SpawnBot01(15.0f, spawnY, true);
				waveSpawned = 1;
			}
		} else {
			// STANDARD: timer-based, one bot per interval
			bot01SpawnTimer += deltaTime;
			if (bot01SpawnTimer >= bot01SpawnInterval) {
				bot01SpawnTimer = 0.0f;
				float spawnY = -3.5f + (rand() % 701) / 100.0f;
				SpawnBot01(15.0f, spawnY, false);
				waveSpawned++;
			}
		}
	}

	for (int i = 0; i < (int)bot01Positions.size(); i++) {
		// Missile knockback — decays fast, kept separate so it doesn't fight the chase spring
		bot01Positions[i].x += bot01XKnockbackVels[i] * deltaTime;
		bot01XKnockbackVels[i] *= expf(-9.0f * deltaTime);
		if (fabsf(bot01XKnockbackVels[i]) < 0.01f) bot01XKnockbackVels[i] = 0.0f;

		// Arrival behavior: rush in fast from far, decelerate to a slow tracking hover
		// once close — classic Craig Reynolds arrival steering.
		// kSlowingRadius defines where braking begins; kNearSpeed is the close-range cap.
		static constexpr float kSlowingRadius = 14.0f;
		static constexpr float kFarSpeed      = 7.0f;
		static constexpr float kNearSpeed     = 0.6f;
		float dx   = playerX - bot01Positions[i].x;
		float dy   = playerY - bot01Positions[i].y;
		float dist = sqrtf(dx * dx + dy * dy);
		float t    = dist / kSlowingRadius;
		if (t > 1.0f) t = 1.0f;
		float dynamicMaxSpeed = kNearSpeed + (kFarSpeed - kNearSpeed) * t;

		// X spring chase + gentle leftward drift so bots eventually leave the screen
		float xForce = bot01SteerForce * dx
		             - bot01YDamping   * bot01XVelocities[i]
		             - 1.5f;
		bot01XVelocities[i] += xForce * deltaTime;
		if (bot01XVelocities[i] >  dynamicMaxSpeed) bot01XVelocities[i] =  dynamicMaxSpeed;
		if (bot01XVelocities[i] < -dynamicMaxSpeed) bot01XVelocities[i] = -dynamicMaxSpeed;
		bot01Positions[i].x += bot01XVelocities[i] * deltaTime;

		// Y spring chase
		float yForce = bot01SteerForce * dy
		             - bot01YDamping   * bot01YVelocities[i];
		bot01YVelocities[i] += yForce * deltaTime;
		if (bot01YVelocities[i] >  dynamicMaxSpeed) bot01YVelocities[i] =  dynamicMaxSpeed;
		if (bot01YVelocities[i] < -dynamicMaxSpeed) bot01YVelocities[i] = -dynamicMaxSpeed;
		bot01Positions[i].y += bot01YVelocities[i] * deltaTime;
	}
	for (int i = (int)bot01Positions.size() - 1; i >= 0; i--) {
		if (bot01Positions[i].x < -15.0f) {
			bot01Positions          .erase(bot01Positions.begin()          + i);
			bot01XVelocities        .erase(bot01XVelocities.begin()        + i);
			bot01YVelocities        .erase(bot01YVelocities.begin()        + i);
			bot01XKnockbackVels     .erase(bot01XKnockbackVels.begin()     + i);
			bot01HP                 .erase(bot01HP.begin()                 + i);
			bot01HitTimers          .erase(bot01HitTimers.begin()          + i);
			bot01HasShield          .erase(bot01HasShield.begin()          + i);
			bot01ShieldActive       .erase(bot01ShieldActive.begin()       + i);
			bot01ShieldOnCooldown   .erase(bot01ShieldOnCooldown.begin()   + i);
			bot01ShieldTimer        .erase(bot01ShieldTimer.begin()        + i);
			bot01ShieldCooldownTimer.erase(bot01ShieldCooldownTimer.begin() + i);
		}
	}

	for (int i = 0; i < (int)bot01HitTimers.size(); i++) {
		if (bot01HitTimers[i] > 0.0f) {
			bot01HitTimers[i] -= deltaTime;
			if (bot01HitTimers[i] < 0.0f) bot01HitTimers[i] = 0.0f;
		}
	}

	for (int i = (int)debris.size() - 1; i >= 0; i--) {
		debris[i].pos      += debris[i].vel * deltaTime;
		debris[i].angle    += debris[i].spinSpeed * deltaTime;
		debris[i].lifetime -= deltaTime;
		if (debris[i].lifetime <= 0.0f)
			debris.erase(debris.begin() + i);
	}
}

void Bot01::UpdateShields(float deltaTime, const std::vector<Vec3>& missilePositions) {
	for (int i = 0; i < (int)bot01Positions.size(); i++) {
		if (!bot01HasShield[i]) continue;

		if (bot01ShieldActive[i]) {
			bot01ShieldTimer[i] += deltaTime;
			if (bot01ShieldTimer[i] >= kShieldDuration) {
				bot01ShieldActive[i]        = false;
				bot01ShieldOnCooldown[i]    = true;
				bot01ShieldCooldownTimer[i] = 0.0f;
				bot01ShieldTimer[i]         = 0.0f;
			}
		} else if (bot01ShieldOnCooldown[i]) {
			bot01ShieldCooldownTimer[i] += deltaTime;
			if (bot01ShieldCooldownTimer[i] >= kShieldCooldown) {
				bot01ShieldOnCooldown[i]    = false;
				bot01ShieldCooldownTimer[i] = 0.0f;
			}
		} else {
			// Activate when any missile enters proximity range
			const Vec3& pos = bot01Positions[i];
			for (const Vec3& mPos : missilePositions) {
				float dx = mPos.x - pos.x;
				float dy = mPos.y - pos.y;
				if (dx * dx + dy * dy < kShieldProximity * kShieldProximity) {
					bot01ShieldActive[i] = true;
					bot01ShieldTimer[i]  = 0.0f;
					break;
				}
			}
		}
	}
}

void Bot01::Render(Shader* shader,
	const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const {
	glUniformMatrix4fv(shader->GetUniformID("projectionMatrix"), 1, GL_FALSE, projectionMatrix);
	glUniformMatrix4fv(shader->GetUniformID("viewMatrix"), 1, GL_FALSE, viewMatrix);

	// Bot01 body — red, flashes white on hit
	for (int i = 0; i < (int)bot01Positions.size(); i++) {
		if (bot01HitTimers[i] > 0.0f)
			glUniform4f(shader->GetUniformID("color"), 1.0f, 1.0f, 1.0f, 1.0f);
		else
			glUniform4f(shader->GetUniformID("color"), 1.0f, 0.1f, 0.1f, 1.0f);
		Matrix4 m = MMath::translate(bot01Positions[i]) *
			MMath::rotate(180.0f, Vec3(0.0f, 1.0f, 0.0f)) *
			MMath::scale(0.17f, 0.17f, 0.17f);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
		bot01Mesh->Render();
	}

	// Bot01 thrust — additive blue electric pulse (faster than player's orange)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDepthMask(GL_FALSE);
	glUniform1f(shader->GetUniformID("emissive"), 1.0f);
	float pulse = 0.65f + 0.25f * sinf(thrustTimer * 20.0f)
	                    + 0.10f * sinf(thrustTimer * 37.0f);
	glUniform4f(shader->GetUniformID("color"), 0.0f, 0.4f * pulse, 1.0f, pulse);
	for (int i = 0; i < (int)bot01Positions.size(); i++) {
		Matrix4 m = MMath::translate(bot01Positions[i]) *
			MMath::rotate(180.0f, Vec3(0.0f, 1.0f, 0.0f)) *
			MMath::scale(0.17f, 0.17f, 0.17f);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
		bot01ThrustMesh->Render();
	}
	glUniform1f(shader->GetUniformID("emissive"), 0.0f);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	// Shielded bot shields — Fresnel emissive bubble in orange (distinct from player's cyan)
	bool anyShield = false;
	for (int i = 0; i < (int)bot01ShieldActive.size(); i++)
		if (bot01ShieldActive[i]) { anyShield = true; break; }

	if (anyShield) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glUniform1f(shader->GetUniformID("emissive"), 1.0f);
		glUniform4f(shader->GetUniformID("color"), 1.0f, 0.35f, 0.0f, 1.0f);
		for (int i = 0; i < (int)bot01Positions.size(); i++) {
			if (!bot01ShieldActive[i]) continue;
			Matrix4 sm = MMath::translate(bot01Positions[i]) *
			             MMath::rotate(180.0f, Vec3(0.0f, 1.0f, 0.0f)) *
			             MMath::scale(0.17f, 0.17f, 0.17f);
			glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, sm);
			shieldMesh->Render();
		}
		glUniform1f(shader->GetUniformID("emissive"), 0.0f);
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}

	// Debris
	if (!debris.empty()) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glDepthMask(GL_FALSE);
		glUniform1f(shader->GetUniformID("emissive"), 1.0f);
		for (int i = 0; i < (int)debris.size(); i++) {
			float alpha = debris[i].lifetime / debris[i].maxLifetime;
			glUniform4f(shader->GetUniformID("color"),
				debris[i].color.x, debris[i].color.y, debris[i].color.z, alpha);
			float ps = debris[i].pieceScale;
			Matrix4 m = MMath::translate(debris[i].pos) *
			            MMath::rotate(debris[i].angle, Vec3(0.0f, 0.0f, 1.0f)) *
			            MMath::scale(ps, ps, ps);
			glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
			fragmentMesh->Render();
		}
		glUniform1f(shader->GetUniformID("emissive"), 0.0f);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}
}

bool Bot01::DamageBot01(int index, int amount) {
	bot01HP[index] -= amount;
	if (bot01HP[index] <= 0) {
		SpawnKillDebris(bot01Positions[index], Vec3(1.0f, 0.2f, 0.2f), amount > 1 ? 10 : 6);
		bot01Positions          .erase(bot01Positions.begin()          + index);
		bot01XVelocities        .erase(bot01XVelocities.begin()        + index);
		bot01YVelocities        .erase(bot01YVelocities.begin()        + index);
		bot01XKnockbackVels     .erase(bot01XKnockbackVels.begin()     + index);
		bot01HP                 .erase(bot01HP.begin()                 + index);
		bot01HitTimers          .erase(bot01HitTimers.begin()          + index);
		bot01HasShield          .erase(bot01HasShield.begin()          + index);
		bot01ShieldActive       .erase(bot01ShieldActive.begin()       + index);
		bot01ShieldOnCooldown   .erase(bot01ShieldOnCooldown.begin()   + index);
		bot01ShieldTimer        .erase(bot01ShieldTimer.begin()        + index);
		bot01ShieldCooldownTimer.erase(bot01ShieldCooldownTimer.begin() + index);
		return true;
	}
	bot01HitTimers[index] = 0.18f;
	SpawnHitDebris(bot01Positions[index], Vec3(1.0f, 0.95f, 0.3f), amount > 1 ? 6 : 3);
	return false;
}

void Bot01::RemoveBot01(int index) {
	SpawnKillDebris(bot01Positions[index], Vec3(1.0f, 0.2f, 0.2f), 6);
	bot01Positions          .erase(bot01Positions.begin()          + index);
	bot01XVelocities        .erase(bot01XVelocities.begin()        + index);
	bot01YVelocities        .erase(bot01YVelocities.begin()        + index);
	bot01XKnockbackVels     .erase(bot01XKnockbackVels.begin()     + index);
	bot01HP                 .erase(bot01HP.begin()                 + index);
	bot01HitTimers          .erase(bot01HitTimers.begin()          + index);
	bot01HasShield          .erase(bot01HasShield.begin()          + index);
	bot01ShieldActive       .erase(bot01ShieldActive.begin()       + index);
	bot01ShieldOnCooldown   .erase(bot01ShieldOnCooldown.begin()   + index);
	bot01ShieldTimer        .erase(bot01ShieldTimer.begin()        + index);
	bot01ShieldCooldownTimer.erase(bot01ShieldCooldownTimer.begin() + index);
}

void Bot01::ResetWave() {
	currentWaveIndex = (currentWaveIndex + 1) % kWaveCount;
	const Bot01WaveDesc& w = kWaves[currentWaveIndex];
	currentWaveType = w.type;
	waveSize        = w.count;
	waveSpawned     = 0;
	bot01SpawnTimer = 0.0f;
}

void Bot01::Reset() {
	bot01Positions.clear();
	bot01XVelocities.clear();
	bot01YVelocities.clear();
	bot01XKnockbackVels.clear();
	bot01HP.clear();
	bot01HitTimers.clear();
	bot01HasShield.clear();
	bot01ShieldActive.clear();
	bot01ShieldOnCooldown.clear();
	bot01ShieldTimer.clear();
	bot01ShieldCooldownTimer.clear();
	debris.clear();
	bot01SpawnTimer  = 0.0f;
	totalTime        = 0.0f;
	waveSpawned      = 0;
	currentWaveIndex = 0;
	currentWaveType  = Bot01WaveType::STANDARD;
	waveSize         = 0;
}

void Bot01::TriggerWave(Bot01WaveType type, int count, float interval) {
	currentWaveType    = type;
	waveSize           = count;
	waveSpawned        = 0;
	bot01SpawnTimer    = 0.0f;
	bot01SpawnInterval = interval;
}
