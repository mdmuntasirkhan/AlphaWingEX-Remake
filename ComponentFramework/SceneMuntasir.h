#pragma once
#ifndef SCENEMUNTASIR_H
#define SCENEMUNTASIR_H
#include "Scene.h"
#include "Vector.h"
#include <Matrix.h>
#include "Sound.h"
#include "Player.h"
#include "Asteroid.h"
#include "Bot01.h"
#include "Bot02.h"
#include "Bullet.h"
#include "Environment.h"
#include "SaveData.h"
#include "SceneSwitcher.h"
#include "LevelDirector.h"
#include "ShardBeacon.h"
#include "DebugOverlay.h"
#include <vector>

using namespace MATH;
 
union SDL_Event;
class Shader;

class SceneMuntasir : public Scene {
private:
	// Level scripting — environment mesh chunks, timeline events
	LevelDirector* levelDirector;


	// Assets
	Player* player;
	Asteroid* asteroid;
	Bot01* bot01;
	Bot02* bot02;
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


	// Energy shards — collectible RPG currency dropped by enemies
	struct Shard {
		Vec3  pos{};
		Vec3  vel{};
		float angle     = 0.0f;
		float spinSpeed = 0.0f;
	};
	std::vector<Shard> shards;
	int   shardCount;
	Mesh* shardMesh;


	// Lost shard beacon — satellite placed at death position, recoverable next run
	ShardBeacon* shardBeacon;
	float beaconTriggerTime;
	int  prevLives;


	// Collecting radius
	static constexpr float kMagnetRadius  = 2.4f;
	static constexpr float kCollectRadius = 0.5f;


	void SpawnShards(const Vec3& pos, int count);
	void SaveGame();


	// DrawGui HUD
	void DrawHUD();
	void DrawPauseMenu();
	void DrawGameOver();
	void PlayHoverSound();


	// Auto save
	float autoSaveTimer;
	static constexpr float kAutoSaveInterval = 10.0f;


	// Phase-based enemy progression — driven by PHASE_CHANGE events in the level script
	int currentPhase;


	// Explosion cooldown
	float explosionCooldown;
	float explosionCooldownTimer;


	// Audio
	SDL_AudioStream* bgmPlayer;
	SDL_AudioStream* sfxPlayer;
	SDL_AudioStream* sfxLaserHitStream;
	Sound* sfxLaser;
	Sound* sfxLaserHit;
	Sound* sfxExplosion;
	Sound* sfxMissileHit;
	Sound* sfxShieldHit;
	Sound* bgmMusic;
	float musicVolume;
	float sfxVolume;
	bool musicPaused;


	// Cooldown prevents sound spam when bullets bounce rapidly off shield
	float shieldHitCooldownTimer;
	static constexpr float kShieldHitCooldown = 0.4f;


	// Pause
	bool gamePaused;
	bool pauseShowSettings;


	// video settings uncommitted until APPLY is pressed
	int  pendingResIndex;
	bool pendingFullscreen;
	int  pendingVsync;
	int  pendingTargetFPS;


	// Debug overlay — toggled with F9
	DebugOverlay* debugOverlay;
	bool          showDebugOverlay;


	// Hold Q to warp
	bool  Q_Held;
	float Q_HoldTimer;


	// Smoothly restores full player speed after warp ends
	bool  prevWarping;
	float postWarpTimer;
	static constexpr float kPostWarpEaseDuration = 2.5f;


	// F10 toggles Camera debug window
	bool  showCameraDebug;
	float debugCameraFOV;
	float currentAspect;


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
	virtual void OnVideoChanged(int w, int h) override;
	virtual void DrawGui() override;
};

#endif // SCENEMUNTASIR_H
