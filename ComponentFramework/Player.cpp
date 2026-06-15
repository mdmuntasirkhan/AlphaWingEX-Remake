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

	// Attributes
	velocity = Vec3(0.0f, 0.0f, 0.0f);
	thrustPower = 15.0f;
	friction = 8.0f;
	maxSpeed = 10.0f;

	// Shield Mesh
	shieldMesh = new Mesh("meshes/Temp_AlphaWing_Shield.obj");
	if (shieldMesh->OnCreate() == false) {
		std::cout << "Shield mesh not found!\n";
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
	float speed = sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
	if (speed > maxSpeed) {
		velocity = velocity * (maxSpeed / speed);
	}

	// Move the position
	pos += velocity * deltaTime;

	// Screen Boundary
	if (pos.x < -8.0f) { pos.x = -8.0f; velocity.x = 0.0f; }
	if (pos.x >  8.0f) { pos.x =  8.0f; velocity.x = 0.0f; }
	if (pos.y < -4.5f) { pos.y = -4.5f; velocity.y = 0.0f; }
	if (pos.y >  4.5f) { pos.y =  4.5f; velocity.y = 0.0f; }


	// Shield
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

	// Send matrices to shader
	glUniformMatrix4fv(shader->GetUniformID("projectionMatrix"), 1, GL_FALSE, projectionMatrix);
	glUniformMatrix4fv(shader->GetUniformID("viewMatrix"), 1, GL_FALSE, viewMatrix);
	glUniformMatrix4fv(shader->GetUniformID("modelMatrix"),	1, GL_FALSE, modelMatrix);

	// Alpha Wing Mesh
	mesh->Render();

	// Shield Mesh
	if (shieldActive) {
		// Turn ON transparancy
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);

		glUniform1f(shader->GetUniformID("emissive"), 1.0f);  // skip lighting
		glUniform4f(shader->GetUniformID("color"), 0.0f, 0.9f, 1.0f, 0.25f);

		Matrix4 shieldMatrix = MMath::translate(pos) *
							   MMath::scale(0.3f, 0.3f, 0.3f);

		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"),
							   1, GL_FALSE, shieldMatrix);
		
		//glUniform1f(shader->GetUniformID("alphaValue"), 0.2f);	// 20% visible

		// Shield Mesh
		shieldMesh->Render();

		// Reset alpha back to 1.0 for everything else
		//glUniform1f(shader->GetUniformID("alphaValue"), 1.0f);
		glUniform1f(shader->GetUniformID("emissive"), 0.0f);

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