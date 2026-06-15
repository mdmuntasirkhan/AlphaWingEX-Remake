#include "Enemy.h"
#include <MMath.h>
#include <glew.h>
#include <iostream>
#include <cstdlib>

Enemy::Enemy() :
	asteroidMesh{ nullptr },
	bot01Mesh{ nullptr },
	asteroidSpeed{ 2.0f },
	bot01Speed{ 3.0f },
	asteroidSpawnTimer{ 0.0f },
	asteroidSpawnInterval{ 1.5f },
	bot01SpawnTimer{ 0.0f },
	bot01SpawnInterval{ 2.0f },
	totalTime{ 0.0f } 
{
	// Leave Empty
}

Enemy::~Enemy() {}

bool Enemy::OnCreate(const char* asteroidFile,
					 const char* bot01File) {
	// Asteroid mesh
	asteroidMesh = new Mesh(asteroidFile);
	if (asteroidMesh->OnCreate() == false) {
		std::cout << " Asteroid mesh not found!\n";
		return false;
	}

	// Bot01 mesh
	bot01Mesh = new Mesh(bot01File);
	if (bot01Mesh->OnCreate() == false) {
		std::cout << "Bot01 mesh not found!\n";
		return false;
	}

	return true;

}

void Enemy::OnDestroy() {
	if (asteroidMesh) {
		asteroidMesh->OnDestroy();
		delete asteroidMesh;
		asteroidMesh = nullptr;
	}
	if (bot01Mesh) {
		bot01Mesh->OnDestroy();
		delete bot01Mesh;
		bot01Mesh = nullptr;
	}
	asteroidPositions.clear();
	bot01Positions.clear();
}

void Enemy::Update(float deltaTime) {
	totalTime += deltaTime;

	// Wave 1 - Asteroid always spawn
	asteroidSpawnTimer += deltaTime;
	if (asteroidSpawnTimer >= asteroidSpawnInterval) {
		asteroidSpawnTimer = 0.0f;
		float randomY = ((rand() % 10) - 5) * 0.5f;
		asteroidPositions.push_back(Vec3(15.0f, randomY, -10.0f));
	}

	// Move as teroid left
	for (int i = 0; i < asteroidPositions.size(); i++) {
		asteroidPositions[i].x -= asteroidSpeed * deltaTime;
	}

	// Remove asteroid off screen
	for (int i = asteroidPositions.size() - 1; i >= 0; i--) {
		if (asteroidPositions[i].x < -15.0f) {
			asteroidPositions.erase(asteroidPositions.begin() + i );
		}
	}

	// Wave 2 Bot01 spawns after 30 seconsds
	if (totalTime > 30.0f) {
		bot01SpawnTimer += deltaTime;
		if (bot01SpawnTimer >= bot01SpawnInterval) {
			bot01SpawnTimer = 0.0f;
			float randomY = ((rand() % 10) - 5) * 0.5f;
			bot01Positions.push_back(Vec3(15.0f, randomY, -10.0f));
		}
	}

	// Move bot left
	for (int i = 0; i < bot01Positions.size(); i++) {
		bot01Positions[i].x -= bot01Speed * deltaTime;
	}

	// Remove Bot01 off the screen
	for (int i = bot01Positions.size() - 1; i >= 0; i--) {
		if (bot01Positions[i].x < -15.0f) {
			bot01Positions.erase(bot01Positions.begin() + i);
		}
	}
}

void Enemy::Render(Shader* shader,
	const Matrix4& projectionMatrix,
	const Matrix4& viewMatrix) const {
	glUniformMatrix4fv(shader->GetUniformID("projectionMatrix"),
		1, GL_FALSE, projectionMatrix);
	glUniformMatrix4fv(shader->GetUniformID("viewMatrix"),
		1, GL_FALSE, viewMatrix);

	// Draw asteroids
	for (int i = 0; i < asteroidPositions.size(); i++) {
		Matrix4 asteroidMatrix = MMath::translate(asteroidPositions[i]) *
			MMath::scale(0.4f, 0.4f, 0.4f);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"),
			1, GL_FALSE, asteroidMatrix);
		asteroidMesh->Render();
	}

	// Draw Bot01
	for (int i = 0; i < bot01Positions.size(); i++) {
		Matrix4 bot01Matrix = MMath::translate(bot01Positions[i]) *
			MMath::rotate(180.0f, Vec3(0.0f, 1.0f, 0.0f)) *
			MMath::scale(0.3f, 0.3f, 0.3f);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"),
			1, GL_FALSE, bot01Matrix);
		bot01Mesh->Render();
	}
}

void Enemy::RemoveAsteroid(int index) {
	asteroidPositions.erase(asteroidPositions.begin() + index);
}

void Enemy::RemoveBot01(int index) {
	bot01Positions.erase(bot01Positions.begin() + index);
}

void Enemy::Reset() {
	asteroidPositions.clear();
	bot01Positions.clear();
	asteroidSpawnTimer = 0.0f;
	bot01SpawnTimer = 0.0f;
	totalTime = 0.0f;
}