#include "Player.h"
#include "Mesh.h"
#include <MMath.h>
#include <glew.h>
#include <iostream>

Player::Player() :  mesh { nullptr },
					speed { 5.0f }, 
					health { 100 }, 
					maxHealth { 100 }, 
					lives { 3 },
					shieldActive { false },
					shieldTimer { 0.0f },
					shieldDuration { 2.0f },
					shieldCooldown { 8.0f },
					shieldCooldownTimer { 0.0f },
					shieldOnCooldown { false },
					shieldMesh { nullptr }


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

	// Shield Mesh
	shieldMesh = new Mesh("meshes/Temp_AlphaWing_Shield.obj");
	if (shieldMesh->OnCreate() == false) {
		std::cout << "Shield mesh not found!\n";
		return false;
	}

	pos = Vec3(0.0f, 0.0f, -10.0f);
	modelMatrix = MMath::translate(pos) *
				  MMath::scale(0.3f, 0.3f, 0.3f);
	return true;
}

void Player::OnDestroy() {
	// Alpha Wing Mesh
	if (mesh) {
		mesh->OnDestroy();
		delete mesh;
		mesh = nullptr;
	}
	// Shield Mesh
	if (shieldMesh) {
		shieldMesh->OnDestroy();
		delete shieldMesh;
		shieldMesh = nullptr;
	}
}

void Player::HandleEvents(const SDL_Event& sdlEvent) {
	if (sdlEvent.type == SDL_EVENT_KEY_DOWN) {
		switch (sdlEvent.key.scancode) {
		case SDL_SCANCODE_W:
			pos.y += speed * 0.1f;
			if (pos.y > 4.5f) pos.y = 4.5f;
			break;
		case SDL_SCANCODE_S:
			pos.y -= speed * 0.1f;
			if (pos.y < -4.5f) pos.y = -4.5f;
			break;
		case SDL_SCANCODE_A:
			pos.x -= speed * 0.1f;
			if (pos.x < -8.0f) pos.x = -8.0f;
			break;
		case SDL_SCANCODE_D:
			pos.x += speed * 0.1f;
			if (pos.x > 8.0f) pos.x = 8.0f;
			break;
		case SDL_SCANCODE_E:
			ActivateShield();
			break;
		default:
			break;
		}
	}
}

void Player::Update(float deltaTime) {

	modelMatrix = MMath::translate(pos) *
				  MMath::scale(0.3f, 0.3f, 0.3f);

	// Shield active countdown
	if (shieldActive) {
		shieldTimer += deltaTime;
		if (shieldTimer >= shieldDuration) {
			// Shield ran out
			shieldActive = false;
			shieldOnCooldown = true;
			shieldCooldownTimer = 0.0f;
		}
	}

	// Shield recharging
	if (shieldOnCooldown) {
		shieldCooldownTimer += deltaTime;
		if (shieldCooldownTimer >= shieldCooldown) {
			// Fully recharged
			shieldOnCooldown = false;
			shieldCooldownTimer = 0.0f;
		}
	}
}

void Player::Render(Shader* shader,
					const Matrix4& projectionMatrix,
					const Matrix4& viewMatrix) const {
	glUniformMatrix4fv(shader->GetUniformID("projectionMatrix"),
					1, GL_FALSE, projectionMatrix);
	glUniformMatrix4fv(shader->GetUniformID("viewMatrix"),
					1, GL_FALSE, viewMatrix);
	glUniformMatrix4fv(shader->GetUniformID("modelMatrix"),
					1, GL_FALSE, modelMatrix);
	// Alpha Wing Mesh
	mesh->Render();

	// Shield Mesh
	if (shieldActive) {
		// Turn ON transparancy
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);

		Matrix4 shieldMatrix = MMath::translate(pos) *
							   MMath::scale(1.0f, 1.0f, 1.0f);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"),
							   1, GL_FALSE, shieldMatrix);
		glUniform1f(shader->GetUniformID("alphaValue"), 0.3f);	// 30% visible

		// Shield Mesh
		shieldMesh->Render();

		// Reset alpha back to 1.0 for everything else
		glUniform1f(shader->GetUniformID("alphaValue"), 1.0f);

		// Turn OFF transparancy
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}
}

void Player::TakeDamage(float amount) {
	//Shield blocks all damage!
	if (shieldActive) return;

	health -= amount;
	if (health <= 0.0f) {
		lives--;
		health = maxHealth;
	}
}

void Player::ActivateShield() {
	// Only active if not already active and not on cooldown
	if (!shieldActive && !shieldOnCooldown) {
		shieldActive = true;
		shieldTimer = 0.0f;
	}
}

void Player::Reset() {
	health = maxHealth;
	lives = 3;
	pos = Vec3(0.0f, 0.0f, -10.0f);
}