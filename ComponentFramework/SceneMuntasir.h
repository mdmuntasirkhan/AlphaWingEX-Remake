#pragma once
#ifndef SceneMuntasir_H
#define SceneMuntasir_H
#include "Scene.h"
#include "Vector.h"
#include <Matrix.h>
#include "Sound.h"
#include "Player.h"
#include "Enemy.h"
#include "Bullet.h"
#include "Environment.h"
#include "SaveData.h"
#include "SceneSwitcher.h"
#include <vector>

using namespace MATH;
 
union SDL_Event;
class Shader;

class SceneMuntasir : public Scene {
private:
	// Game Class
	Player* player;
	Enemy* enemy;
	Bullet* bullet;
	Environment* environment;

	// Shader and Camera
	Shader* shader;
	Matrix4 projectionMatrix;
	Matrix4 viewMatrix;
	bool drawInWireMode;

	// Game state
	bool gameOver;
	int score;

	// Energy shards — RPG currency dropped by enemies
	struct Shard {
		Vec3  pos;
		Vec3  vel;
		float angle;
		float spinSpeed;
	};
	std::vector<Shard> shards;
	int   shardCount;
	Mesh* shardMesh;

	// Lost shard pile — dropped on death, recoverable once
	struct DroppedShard {
		Vec3  pos;
		int   count;
		float pulseTimer;
	};
	DroppedShard lostShards;
	bool hasLostShards;
	int  prevLives;

	static constexpr float kMagnetRadius  = 2.4f;
	static constexpr float kCollectRadius = 0.5f;

	void SpawnShards(const Vec3& pos, int count);
	void SaveGame();

	// Auto-save
	float autoSaveTimer;
	static constexpr float kAutoSaveInterval = 10.0f; // save every 10 s

	// Explosion cooldown
	float explosionCooldown;
	float explosionCooldownTimer;

	// Audio
	SDL_AudioStream* audioPlayer;
	SDL_AudioStream* sfxPlayer;
	Sound* sfxLaser;
	Sound* sfxExplosion;
	Sound* audioTest;
	float musicVolume;
	float sfxVolume;
	bool musicPaused;

	// Pause
	bool gamePaused;
	bool pauseShowSettings;

	// Hover — selectSound played on a low-gain stream
	SDL_AudioStream* hoverStream;
	Sound*           uiClickSound;
	unsigned int     lastHoveredId;
	Uint64           lastHoverTick;

public:
	explicit SceneMuntasir();
	virtual ~SceneMuntasir();

	virtual bool OnCreate() override;
	virtual void OnDestroy() override;
	virtual void Update(const float deltaTime) override;
	virtual void RenderBackground() override;
	virtual void Render() const override;
	virtual void HandleEvents(const SDL_Event& sdlEvent) override;

	// ImGui
	virtual void DrawGui() override;
};


#endif // SCENE0_H