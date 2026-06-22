#include "Player.h"
#include "Mesh.h"
#include <MMath.h>
#include <glew.h>
#include <iostream>

Player::Player() :  mesh { nullptr },
					cockpitMesh { nullptr },
					attachmentMesh { nullptr },
					thrustMesh { nullptr },
					speed { 5.0f }, 
					health { 100 }, 
					maxHealth { 100 }, 
					lives { 3 },
					velocity{ 0.0f, 0.0f, 0.0f },
					thrustPower { 15.0f },
					friction { 8.0f },
					maxSpeed { 10.0f },
					shieldActive { false },
					shieldTimer { 0.0f },
					shieldDuration { 10.0f },
					shieldRechargeRate { 1.0f },
					shieldMesh { nullptr },
					shieldSweepTimer { 0.0f },
					shieldSweepPeriod { 1.2f },
					shieldYRadius { 2.5f },
					shieldZRadius { 3.0f },
					shieldGlowRadius { 1.4f },
					rollAngle        { 0.0f },
					rollVelocity     { 0.0f },
					rollStiffness    { 100.0f },
					rollDamping      { 16.0f },
					maxRollAngle     { 5.0f },
					thrustTimer      { 0.0f }
{
	// leave Empty
}

Player::~Player() {}

bool Player::OnCreate(const char* meshFile) {
	// Alpha Wing Mesh
	mesh = new Mesh(meshFile);
	if (mesh->OnCreate() == false) {
		std::cout << "Player mesh not found!\n";
		return false;
	}

	// Cockpit mesh
	cockpitMesh = new Mesh("meshes/Temp_AlphaWingEX_Cockpit.obj");
	if (cockpitMesh->OnCreate() == false) {
		std::cout << "Cockpit mesh not found!\n";
		return false;
	}

	// Magnet attachment mesh
	attachmentMesh = new Mesh("meshes/Temp_AlphaWingEX_Attachment01.obj");
	if (attachmentMesh->OnCreate() == false) {
		std::cout << "Attachment mesh not found!\n";
		return false;
	}

	// Shield Mesh
	shieldMesh = new Mesh("meshes/Temp_AlphaWing_Shield.obj");
	if (shieldMesh->OnCreate() == false) {
		std::cout << "Shield mesh not found!\n";
		return false;
	}

	// Thrust Mesh
	thrustMesh = new Mesh("meshes/Temp_AlphaWing_Thrust.obj");
	if (thrustMesh->OnCreate() == false) {
		std::cout << "Thrust mesh not found!\n";
		return false;
	}

	pos = Vec3(0.0f, 0.0f, -10.0f);
	modelMatrix = MMath::translate(pos) *
				  MMath::scale(0.5f, 0.5f, 0.5f);
	return true;
}

void Player::OnDestroy() {
	// Alpha Wing Mesh
	if (mesh) {
		mesh->OnDestroy();
		delete mesh;
		mesh = nullptr;
	}
	if (cockpitMesh) {
		cockpitMesh->OnDestroy();
		delete cockpitMesh;
		cockpitMesh = nullptr;
	}
	if (attachmentMesh) {
		attachmentMesh->OnDestroy();
		delete attachmentMesh;
		attachmentMesh = nullptr;
	}
	// Shield Mesh
	if (shieldMesh) {
		shieldMesh->OnDestroy();
		delete shieldMesh;
		shieldMesh = nullptr;
	}

	// Thrust Mesh
	if (thrustMesh) {
		thrustMesh->OnDestroy();
		delete thrustMesh;
		thrustMesh = nullptr;
	}
}

void Player::HandleEvents(const SDL_Event& sdlEvent) {
	if (sdlEvent.type == SDL_EVENT_KEY_DOWN) {
		switch (sdlEvent.key.scancode) {
		case SDL_SCANCODE_E:
			ActivateShield();
			break;
		default:
			break;
		}
	}
}

void Player::Update(float deltaTime) {

	// Movement Logic read keyboard everyframe
	const bool* keys = SDL_GetKeyboardState(nullptr);

	Vec3 inputDir(0.0f, 0.0f, 0.0f);
	if (keys[SDL_SCANCODE_W]) inputDir.y += 1.0f;
	if (keys[SDL_SCANCODE_S]) inputDir.y -= 1.0f;
	if (keys[SDL_SCANCODE_A]) inputDir.x -= 1.0f;
	if (keys[SDL_SCANCODE_D]) inputDir.x += 1.0f;

	// Push the ship (function)
	velocity += inputDir * thrustPower * deltaTime;

	// Friction
	velocity -= velocity * friction * deltaTime;

	// Speed cap and Normalized
	float currentSpeed = sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
	if (currentSpeed > maxSpeed) {
		velocity = velocity * (maxSpeed / currentSpeed);
	}

	// Move the position
	pos += velocity * deltaTime;

	// Screen Boundary — derived from 70° vertical FOV at Z=-10: visible ≈ ±7.0 Y, ±12.45 X
	if (pos.x < -11.0f) { pos.x = -11.0f; velocity.x = 0.0f; }
	if (pos.x >  11.0f) { pos.x =  11.0f; velocity.x = 0.0f; }
	if (pos.y < -6.0f)  { pos.y = -6.0f;  velocity.y = 0.0f; }
	if (pos.y >  6.0f)  { pos.y =  6.0f;  velocity.y = 0.0f; }


	thrustTimer += deltaTime;

	float targetRoll = inputDir.y * maxRollAngle;
	rollVelocity += (-rollStiffness * (rollAngle - targetRoll) - rollDamping * rollVelocity) * deltaTime;
	rollAngle    += rollVelocity * deltaTime;

	modelMatrix = MMath::translate(pos) *
				  MMath::rotate(rollAngle, Vec3(0.0f, 0.0f, 1.0f)) *
				  MMath::scale(0.3f, 0.3f, 0.3f);

	if (shieldActive) {
		shieldTimer += deltaTime;
		if (shieldTimer >= shieldDuration) {
			shieldTimer        = shieldDuration;
			shieldActive       = false;
			shieldRechargeRate = kRecharge90Rate; // expired = worst penalty
		}
	} else if (shieldTimer > 0.0f) {
		shieldTimer -= deltaTime * shieldRechargeRate;
		if (shieldTimer <= 0.0f) {
			shieldTimer        = 0.0f;
			shieldRechargeRate = kBaseRechargeRate; // fully recharged — reset rate
		}
	}
}


void Player::Render(Shader* shader,
					const Matrix4& projectionMatrix,
					const Matrix4& viewMatrix) const {

	// Send matrices to shader
	glUniformMatrix4fv(shader->GetUniformID("projectionMatrix"), 1, GL_FALSE, projectionMatrix);
	glUniformMatrix4fv(shader->GetUniformID("viewMatrix"), 1, GL_FALSE, viewMatrix);
	glUniformMatrix4fv(shader->GetUniformID("modelMatrix"),	1, GL_FALSE, modelMatrix);

	// Ship body — cool metallic silver (Phong shading)
	glUniform4f(shader->GetUniformID("color"), 0.72f, 0.76f, 0.82f, 1.0f);
	mesh->Render();

	// Cockpit — electric blue (Phong)
	glUniform4f(shader->GetUniformID("color"), 0.10f, 0.45f, 1.0f, 1.0f);
	cockpitMesh->Render();

	// Magnet attachment — deep crimson body (Phong)
	glUniform4f(shader->GetUniformID("color"), 0.72f, 0.05f, 0.08f, 1.0f);
	attachmentMesh->Render();

	// Attachment magnetic field — faint cyan additive glow (idle state)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDepthMask(GL_FALSE);
	glUniform1f(shader->GetUniformID("emissive"), 1.0f);
	glUniform4f(shader->GetUniformID("color"), 0.0f, 0.7f, 1.0f, 0.18f);
	attachmentMesh->Render();
	glUniform1f(shader->GetUniformID("emissive"), 0.0f);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	// Thrust glow — additive blend so it lights up rather than paints over
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDepthMask(GL_FALSE);
	// Two overlapping sine waves give an organic, non-repeating flicker
	float pulse = 0.75f + 0.15f * sinf(thrustTimer * 11.0f)
	                    + 0.10f * sinf(thrustTimer *  7.3f);
	glUniform1f(shader->GetUniformID("emissive"), 1.0f);
	glUniform4f(shader->GetUniformID("color"), 1.0f, 0.45f * pulse, 0.05f, pulse);
	thrustMesh->Render();
	glUniform1f(shader->GetUniformID("emissive"), 0.0f);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	// Shield Mesh
	if (shieldActive) {
		// Turn ON transparency; cull back faces so the dome interior is invisible
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		glUniform1f(shader->GetUniformID("emissive"), 1.0f);
		glUniform4f(shader->GetUniformID("color"), 0.0f, 0.9f, 1.0f, 1.0f);

		Matrix4 shieldMatrix = MMath::translate(pos) *
							   MMath::rotate(rollAngle, Vec3(0.0f, 0.0f, 1.0f)) *
							   MMath::scale(0.3f, 0.3f, 0.3f);

		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"),
							   1, GL_FALSE, shieldMatrix);
		
		//glUniform1f(shader->GetUniformID("alphaValue"), 0.2f);	// 20% visible

		// Shield Mesh
		shieldMesh->Render();

		// Reset alpha back to 1.0 for everything else
		//glUniform1f(shader->GetUniformID("alphaValue"), 1.0f);
		glUniform1f(shader->GetUniformID("emissive"), 0.0f);

		// Restore state
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}
}

void Player::TakeDamage(float amount) {
	if (shieldActive) return;
	if (lives <= 0) return;

	health -= amount;
	if (health <= 0.0f) {
		lives--;
		health = maxHealth;
	}
}

void Player::ActivateShield() {
	if (shieldActive) {
		// Deactivate — lock in recharge rate based on usage at this moment
		float usagePct = shieldTimer / shieldDuration;
		if      (usagePct >= 0.9f) shieldRechargeRate = kRecharge90Rate;
		else if (usagePct >= 0.8f) shieldRechargeRate = kRecharge80Rate;
		else                       shieldRechargeRate = kBaseRechargeRate;
		shieldActive = false;
	} else if (shieldTimer < shieldDuration) {
		// Activate — any charge remaining is enough
		shieldActive     = true;
		shieldSweepTimer = 0.0f;
	}
}

void Player::Reset() {
	health = maxHealth;
	lives = 3;
	pos = Vec3(0.0f, 0.0f, -10.0f);
	velocity = Vec3(0.0f, 0.0f, 0.0f);
	rollAngle = 0.0f;
	rollVelocity = 0.0f;
	shieldActive       = false;
	shieldTimer        = 0.0f;
	shieldRechargeRate = kBaseRechargeRate;
	thrustTimer = 0.0f;
}