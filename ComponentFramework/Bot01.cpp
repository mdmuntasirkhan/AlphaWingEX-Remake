#include "Bot01.h"
#include <MMath.h>
#include <glew.h>
#include <cstdlib>
#include <cmath>
#include <iostream>

Bot01::Bot01() :
	bot01Mesh{ nullptr },
	bot01ThrustMesh{ nullptr },
	fragmentMesh{ nullptr },
	thrustTimer{ 0.0f },
	bot01SteerForce{ 3.0f },
	bot01YDamping{ 4.5f },
	bot01YMaxSpeed{ 2.5f },
	bot01Speed{ 1.2f },
	bot01SpawnTimer{ 0.0f },
	bot01SpawnInterval{ 6.0f },
	totalTime{ 0.0f },
	waveSize{ 5 },
	waveSpawned{ 0 }
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
	bot01Positions.clear();
	bot01YVelocities.clear();
	bot01HP.clear();
	bot01HitTimers.clear();
	debris.clear();
}

void Bot01::Update(float deltaTime, float /*playerX*/, float playerY) {
	totalTime   += deltaTime;
	thrustTimer += deltaTime;

	if (waveSpawned < waveSize) {
		bot01SpawnTimer += deltaTime;
		if (bot01SpawnTimer >= bot01SpawnInterval) {
			bot01SpawnTimer = 0.0f;
			// Fixed stack: bottom to top so the column enters as a formation
			static const float stackY[5] = { -3.0f, -1.5f, 0.0f, 1.5f, 3.0f };
			float spawnY = stackY[waveSpawned];
			bot01Positions.push_back(Vec3(15.0f, spawnY, -10.0f));
			bot01YVelocities.push_back(0.0f);
			bot01XKnockbackVels.push_back(0.0f);
			bot01HP.push_back(10);
			bot01HitTimers.push_back(0.0f);
			waveSpawned++;
		}
	}

	for (int i = 0; i < (int)bot01Positions.size(); i++) {
		// Normal march + separate X knockback (decays exponentially so it doesn't
		// fight the AI march — industry approach: keep knockback vector separate)
		bot01Positions[i].x -= bot01Speed * deltaTime;
		bot01Positions[i].x += bot01XKnockbackVels[i] * deltaTime;
		bot01XKnockbackVels[i] *= expf(-9.0f * deltaTime); // ~0.5s full decay
		if (fabsf(bot01XKnockbackVels[i]) < 0.01f) bot01XKnockbackVels[i] = 0.0f;

		float force = bot01SteerForce * (playerY - bot01Positions[i].y)
		            - bot01YDamping  * bot01YVelocities[i];
		bot01YVelocities[i] += force * deltaTime;
		if (bot01YVelocities[i] >  bot01YMaxSpeed) bot01YVelocities[i] =  bot01YMaxSpeed;
		if (bot01YVelocities[i] < -bot01YMaxSpeed) bot01YVelocities[i] = -bot01YMaxSpeed;
		bot01Positions[i].y += bot01YVelocities[i] * deltaTime;
	}
	for (int i = (int)bot01Positions.size() - 1; i >= 0; i--) {
		if (bot01Positions[i].x < -15.0f) {
			bot01Positions      .erase(bot01Positions.begin()       + i);
			bot01YVelocities    .erase(bot01YVelocities.begin()     + i);
			bot01XKnockbackVels .erase(bot01XKnockbackVels.begin()  + i);
			bot01HP             .erase(bot01HP.begin()              + i);
			bot01HitTimers      .erase(bot01HitTimers.begin()       + i);
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
		bot01Positions      .erase(bot01Positions.begin()       + index);
		bot01YVelocities    .erase(bot01YVelocities.begin()     + index);
		bot01XKnockbackVels .erase(bot01XKnockbackVels.begin()  + index);
		bot01HP             .erase(bot01HP.begin()              + index);
		bot01HitTimers      .erase(bot01HitTimers.begin()       + index);
		return true;
	}
	bot01HitTimers[index] = 0.18f;
	SpawnHitDebris(bot01Positions[index], Vec3(1.0f, 0.95f, 0.3f), amount > 1 ? 6 : 3);
	return false;
}

void Bot01::RemoveBot01(int index) {
	SpawnKillDebris(bot01Positions[index], Vec3(1.0f, 0.2f, 0.2f), 6);
	bot01Positions      .erase(bot01Positions.begin()       + index);
	bot01YVelocities    .erase(bot01YVelocities.begin()     + index);
	bot01XKnockbackVels .erase(bot01XKnockbackVels.begin()  + index);
	bot01HP             .erase(bot01HP.begin()              + index);
	bot01HitTimers      .erase(bot01HitTimers.begin()       + index);
}

void Bot01::Reset() {
	bot01Positions.clear();
	bot01YVelocities.clear();
	bot01XKnockbackVels.clear();
	bot01HP.clear();
	bot01HitTimers.clear();
	debris.clear();
	bot01SpawnTimer = 0.0f;
	totalTime       = 0.0f;
	waveSpawned     = 0;
}
