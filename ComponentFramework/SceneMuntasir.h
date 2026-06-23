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

	// Game Class
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

	// Energy shards — RPG currency dropped by enemies
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
	float beaconTriggerTime;  // level time at which beacon activates (0 = inactive or immediate)
	int  prevLives;

	static constexpr float kMagnetRadius  = 2.4f;
	static constexpr float kCollectRadius = 0.5f;

	void SpawnShards(const Vec3& pos, int count);
	void SaveGame();

	// DrawGui helpers — called only from DrawGui() each frame
	void DrawHUD();       // always-visible overlay: score, shards, health, shield, missiles
	void DrawPauseMenu(); // ESC pause panel with audio/video settings
	void DrawGameOver();  // game-over screen with Try Again / Title / Exit buttons
	void PlayHoverSound(); // plays a quiet click when the cursor moves onto a new button

	// Auto-save
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
	Sound* bgmMusic;       // background music track
	float musicVolume;
	float sfxVolume;
	bool musicPaused;

	// Shield hit sound cooldown — prevents spam when bullets bounce rapidly
	float shieldHitCooldownTimer;
	static constexpr float kShieldHitCooldown = 0.4f;

	// Pause
	bool gamePaused;
	bool pauseShowSettings;

	// Pending video settings (uncommitted until APPLY is pressed)
	int  pendingResIndex;
	bool pendingFullscreen;
	int  pendingVsync;
	int  pendingTargetFPS;

	// Debug overlay — toggled with F9
	DebugOverlay* debugOverlay;
	bool          showDebugOverlay;

	// F11 warp charge — hold 3 s to trigger full warp
	bool  f11Held;
	float f11HoldTimer;

	// Post-warp control ease-in — smoothly restores full player speed after warp ends
	bool  prevWarping;
	float postWarpTimer;
	static constexpr float kPostWarpEaseDuration = 2.5f;

	// Camera debug — F10 toggles window; slider live-rebuilds FOV + camera Z
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

	// ImGui
	virtual void DrawGui() override;
};


#endif // SCENEMUNTASIR_H