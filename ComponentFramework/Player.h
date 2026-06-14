#pragma once
#ifndef PLAYER_H
#define PLAYER_H

#include "Mesh.h"
#include "Shader.h"
#include <Matrix.h>
#include <Vector.h>
#include <SDL3/SDL_events.h>

using namespace MATH;

union SDL_Event;

class Player {
private:
	Mesh* mesh;
	Matrix4 modelMatrix;

	Vec3 pos;
	float speed;

	float health;
	float maxHealth;
	int lives;

public:
	Player();
	~Player();

	bool OnCreate(const char* meshFile);
	void OnDestroy();
	void Update(float deltaTime);
	void Render(Shader* shader,
		const Matrix4& projectionMatrix,
		const Matrix4& viewMatrix) const;
	void HandleEvents(const SDL_Event& sdlEvent);

	// Getters
	Vec3 GetPosition() const { return pos; }
	float GetHealth() const { return health; }
	int GetLives() const { return lives; }
	bool IsGameOver() const { return IsGameOver; }

	// Danage and Reset
	void TakeDamage(float amount);
	void Reset();

};

#endif // !PLAYER_H
