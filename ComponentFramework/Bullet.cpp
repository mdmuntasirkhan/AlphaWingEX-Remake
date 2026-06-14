#include "Bullet.h"
#include <MMath.h>
#include <glew.h>
#include <iostream>

Bullet::Bullet() :  mesh{ nullptr },
					speed{ 10.0f },
					missileMesh{ nullptr },
					homingTarget{ nullptr },
					missileSpeed{ 7.0f },
					missileCount{ 5 },
					maxMissiles{ 5 },
					missileReloadTimer{ 0.0f },
					missileReloadInterval{ 3.0f }
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
	missilePositions.clear();
}

void Bullet::Spawn(Vec3 position) {
	positions.push_back(position);
}

void Bullet::SpawnHoming(Vec3 position, Vec3* target) {
	missilePositions.push_back(position);
	homingTarget = target;
}

void Bullet::Update(float deltaTime) {
	// Move regular bullets forward
	for (int i = 0; i < positions.size(); i++) {
		positions[i].x += speed * deltaTime;
	}

	// Remove regular bullets off the screen
	for (int i = positions.size() - 1; i >= 0; i--) {
		if (positions[i].x > 15.0f) {
			positions.erase(positions.begin() + i);
		}
	}

	// Homing missiles track target
	for (int i = 0; i < missilePositions.size(); i++) {
		if (homingTarget != nullptr) {
			// Direction toward target
			float dx = homingTarget->x - missilePositions[i].x;
			float dy = homingTarget->y - missilePositions[i].y;
			float length = sqrt(dx * dx + dy * dy);

			//Normalize direction
			if (length > 0) {
				dx /= length;
				dy /= length;
			}

			// Move towards target
			missilePositions[i].x += dx * missileSpeed * deltaTime;
			missilePositions[i].y += dy * missileSpeed * deltaTime;
		}
	}

	// Remove missiles off screen
	for (int i = missilePositions.size() - 1; i >= 0; i--) {
		if (missilePositions[i].x > 15.0f) {
			missilePositions.erase(missilePositions.begin() + i);
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
	for (int i = 0; i < positions.size(); i++) {
		Matrix4 bulletMatrix = MMath::translate(positions[i]) *
			MMath::scale(0.2f, 0.2f, 0.2f);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"),
			1, GL_FALSE, bulletMatrix);
	}

	// Draw Missiles
	for (int i = 0; i < missilePositions.size(); i++) {
		Matrix4 missileMatrix = MMath::translate(missilePositions[i]) *
								MMath::scale(0.3f, 0.3f, 0.3f);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"),
			1, GL_FALSE, missileMatrix);
		missileMesh->Render();
	}
}

void Bullet::RemoveAt(int index) {
	positions.erase(positions.begin() + index);
}

void Bullet::RemoveMissileAt(int index) {
	missilePositions.erase(missilePositions.begin() + index);
}