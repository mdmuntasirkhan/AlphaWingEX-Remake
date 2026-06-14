#pragma once
#ifndef SceneMuntasir_H
#define SceneMuntasir_H
#include "Scene.h"
#include "Vector.h"
#include <Matrix.h>
#include "Sound.h"
#include "Player.h"
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
	// Player Class
	Player* player;

	Shader* shader;
	Matrix4 projectionMatrix;
	Matrix4 viewMatrix;
	bool drawInWireMode;

	// Bullets
	std::vector<Vec3> bulletPositions;
	float bulletSpeed;
	Mesh* bulletMesh;

	// Bot01
	Mesh* Bot01Mesh;
	std::vector<Vec3> Bot01Positions;
	float Bot01Speed;
	float spawnTimer;
	float spawnInterval;

	// Asteroids
	Mesh* asteroidMesh;
	std::vector<Vec3> asteroidPositions;
	float asteroidSpeed;
	float asteroidSpawnTimer;
	float asteroidSpawnInterval;

	// Game state
	bool gameOver;
	int score;

	// Audio
	SDL_AudioStream* audioPlayer;
	SDL_AudioStream* sfxPlayer;  // separate stream just for sound effects
	Sound* sfxLaser;
	Sound* sfxExplosion;
	Sound* audioTest;

	float musicVolume;
	float sfxVolume;
	bool musicPaused;

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