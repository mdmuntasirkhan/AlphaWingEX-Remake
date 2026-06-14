#pragma once
#ifndef SceneMuntasir_H
#define SceneMuntasir_H
#include "Scene.h"
#include "Vector.h"
#include <Matrix.h>
#include "Sound.h"
#include <vector>

using namespace MATH;

/// Forward declarations 
union SDL_Event;
class Body;
class Mesh;
class Shader;
class Texture;

class SceneMuntasir : public Scene {
private:
	Body* playerShip;
	Shader* shader;
	Mesh* AlphaWingMesh;
	Matrix4 projectionMatrix;
	Matrix4 viewMatrix;
	Matrix4 playerModelMatrix;
	bool drawInWireMode;

	Vec3 playerPos;
	float playerSpeed;

	std::vector<Vec3> bulletPositions;
	float bulletSpeed;
	Mesh* bulletMesh;

	Mesh* Bot01Mesh;
	std::vector<Vec3> Bot01Positions;
	float Bot01Speed;
	float spawnTimer;
	float spawnInterval;

	int lives;
	float health;
	float maxHealth;

	Mesh* asteroidMesh;
	std::vector<Vec3> asteroidPositions;
	float asteroidSpeed;
	float asteroidSpawnTimer;
	float asteroidSpawnInterval;

	bool gameOver;

	int score;
	Sound* sfxExplosion;

	Vec3 lightPos;

	SDL_AudioStream* sfxPlayer;  // separate stream just for sound effects
	Sound* sfxLaser;

	float musicVolume;
	float sfxVolume;
	bool musicPaused;

	SDL_AudioStream* audioPlayer;
	Sound* audioTest;

public:
	explicit SceneMuntasir();
	virtual ~SceneMuntasir();

	virtual bool OnCreate() override;
	virtual void OnDestroy() override;
	virtual void Update(const float deltaTime) override;
	virtual void Render() const override;
	virtual void HandleEvents(const SDL_Event& sdlEvent) override;



	// ImGui
	virtual void DrawGui() override;
};


#endif // SCENE0_H