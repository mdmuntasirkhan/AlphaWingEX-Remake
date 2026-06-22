#include <glew.h>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <SDL.h>
#include <SDL3/SDL_events.h>
#include "SceneMuntasir.h"
#include "GameConstants.h"
#include "Level01Script.h"
#include "Level02Script.h"
#include <MMath.h>
#include "Debug.h"
#include "Mesh.h"
#include "Shader.h"
#include "imgui.h"

// Constructor
SceneMuntasir::SceneMuntasir() :
    levelDirector{ nullptr },
    player{ nullptr },
    asteroid{ nullptr },
    bot01{ nullptr },
    bot02{ nullptr },
    bullet{ nullptr },
    environment{ nullptr },
    shader{ nullptr },
    drawInWireMode{ false },
    gameOver{ false },
    score{ 0 },
    shardCount{ 0 },
    shardMesh{ nullptr },
    shardBeacon{ nullptr },
    beaconTriggerTime{ 0.0f },
    prevLives{ 3 },
    autoSaveTimer{ 0.0f },
    currentPhase{ 1 },
    explosionCooldown{ 0.8f },
    explosionCooldownTimer{ 0.0f },
    shieldHitCooldownTimer{ 0.0f },
    bgmPlayer{ nullptr },
    sfxPlayer{ nullptr },
    sfxLaserHitStream{ nullptr },
    sfxLaser{ nullptr },
    sfxLaserHit{ nullptr },
    sfxExplosion{ nullptr },
    sfxMissileHit{ nullptr },
    sfxShieldHit{ nullptr },
    bgmMusic{ nullptr },
    musicVolume{ 0.1f },
    sfxVolume{ 0.05f },
    musicPaused{ false },
    gamePaused{ false },
    pauseShowSettings{ false },
    pendingResIndex{ SaveData::current.resolutionIndex },
    pendingFullscreen{ SaveData::current.fullscreen },
    pendingVsync{ SaveData::current.vsyncMode },
    pendingTargetFPS{ SaveData::current.targetFPS },
    debugOverlay{ nullptr },
    showDebugOverlay{ false },
    hoverStream{ nullptr },
    uiClickSound{ nullptr },
    lastHoveredId{ 0 },
    lastHoverTick{ 0 } {
    Debug::Info("Created SceneMuntasir: ", __FILE__, __LINE__);
}

// Destructor
SceneMuntasir::~SceneMuntasir() {
    Debug::Info("Deleted SceneMuntasir: ", __FILE__, __LINE__);
}

// OnCreate
bool SceneMuntasir::OnCreate() {
    Debug::Info("Loading assets SceneMuntasir: ", __FILE__, __LINE__);

    // Player
    player = new Player();
    if (player->OnCreate("meshes/Temp_AlphaWingEX.obj") == false) {
        std::cout << "Player failed to load!\n";
        return false;
    }

    // Asteroid
    asteroid = new Asteroid();
    if (asteroid->OnCreate("meshes/Temp_Asteroid.obj") == false) {
        std::cout << "Asteroid failed to load!\n";
        return false;
    }

    // Bot01
    bot01 = new Bot01();
    if (bot01->OnCreate("meshes/Temp_AlphaWing_Enemy_Bot01.obj",
        "meshes/Temp_Asteroid.obj") == false) {
        std::cout << "Bot01 failed to load!\n";
        return false;
    }

    // Bot02
    bot02 = new Bot02();
    if (bot02->OnCreate(
        "meshes/Temp_AlphaWingEX_Bot02.obj",
        "meshes/Temp_AlphaWingEX_Bot02_Cockpit.obj",
        "meshes/Temp_AlphaWingEX_Bot02_Fin.obj",
        "meshes/Temp_AlphaWingEX_Bot02_Thrust.obj",
        "meshes/Temp_Asteroid.obj",
        "meshes/Temp_AlphaWingEX_Bot02_Bullet.obj") == false) {
        std::cout << "Bot02 failed to load!\n";
        return false;
    }

    // Bullet - handles regular bullets and missiles
    bullet = new Bullet();
    if (bullet->OnCreate("meshes/Temp_AlphaWing_Bullet.obj",
        "meshes/Temp_AlphaWing_PlayerShip_Missile.obj") == false) {
        std::cout << "Bullet failed to load!\n";
        return false;
    }

    // Environment - starfield (use actual viewport pixel size, not hardcoded 1920x1080)
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    environment = new Environment();
    if (environment->OnCreate((float)vp[2], (float)vp[3]) == false) {
        std::cout << "Environment failed to load!\n";
        return false;
    }

    // Shard mesh (placeholder: bullet OBJ spun tiny — swap later for a real crystal)
    shardMesh = new Mesh("meshes/Temp_AlphaWing_Bullet.obj");
    if (shardMesh->OnCreate() == false) {
        std::cout << "Shard mesh not found!\n";
        return false;
    }

    // Shader
    shader = new Shader("shaders/alphaWingVert.glsl", "shaders/alphaWingFrag.glsl");
    if (shader->OnCreate() == false) {
        std::cout << "Shader failed!\n";
        return false;
    }

    // Camera
    viewMatrix = MMath::lookAt(
        Vec3(0.0f, 0.0f, 0.0f),
        Vec3(0.0f, 0.0f, -1.0f),
        Vec3(0.0f, 1.0f, 0.0f)
    );
    float aspect = static_cast<float>(SaveData::kResolutionW[SaveData::current.resolutionIndex]) /
                   static_cast<float>(SaveData::kResolutionH[SaveData::current.resolutionIndex]);
    GameConst::ComputeWorldBounds(aspect);
    projectionMatrix = MMath::perspective(70.0f, aspect, 0.1f, 100.0f);

    // Audio Setup
    SDL_AudioSpec defaultSpec;
    defaultSpec.freq = 44100;
    defaultSpec.channels = 2;
    defaultSpec.format = SDL_AUDIO_S16;

    // Music stream
    bgmPlayer = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &defaultSpec, nullptr, nullptr);
    if (!bgmPlayer) std::cout << "Failed to create music player\n";
    SDL_SetAudioStreamGain(bgmPlayer, musicVolume);
    SDL_ResumeAudioStreamDevice(bgmPlayer);

    // SFX stream
    sfxPlayer = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &defaultSpec, nullptr, nullptr);
    if (!sfxPlayer) std::cout << "Failed to create SFX player\n";
    SDL_SetAudioStreamGain(sfxPlayer, sfxVolume);
    SDL_ResumeAudioStreamDevice(sfxPlayer);

    // Dedicated stream for laser hit — cleared before each play for instant response
    sfxLaserHitStream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &defaultSpec, nullptr, nullptr);
    if (!sfxLaserHitStream) std::cout << "Failed to create laser hit stream\n";
    SDL_SetAudioStreamGain(sfxLaserHitStream, sfxVolume * 2.0f);
    SDL_ResumeAudioStreamDevice(sfxLaserHitStream);

    // Music
    bgmMusic = new Sound("audio/music/deadmou5-gg.wav");
    bgmMusic->OnCreate();
    bgmMusic->Play(bgmPlayer);

    // SFX
    sfxLaser = new Sound("audio/sfx/LaserShoot.wav");
    sfxLaser->OnCreate();

    sfxLaserHit = new Sound("audio/sfx/AlphaWingEX_Laser_Hit.wav");
    sfxLaserHit->OnCreate();

    sfxExplosion = new Sound("audio/sfx/Explosion04.wav");
    sfxExplosion->OnCreate();

    sfxMissileHit = new Sound("audio/sfx/missileHit.wav");
    sfxMissileHit->OnCreate();

    sfxShieldHit = new Sound("audio/sfx/AWshieldDamageWarning.wav");
    sfxShieldHit->OnCreate();

    hoverStream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &defaultSpec, nullptr, nullptr);
    if (hoverStream) {
        SDL_SetAudioStreamGain(hoverStream, sfxVolume * 0.35f);
        SDL_ResumeAudioStreamDevice(hoverStream);
    }
    uiClickSound = new Sound("audio/sfx/Select01.wav");
    uiClickSound->OnCreate();

    SDL_Log("Music queued bytes: %d", SDL_GetAudioStreamQueued(bgmPlayer));
    SDL_Log("SFX queued bytes: %d", SDL_GetAudioStreamQueued(sfxPlayer));

    // Restore full game state from save (works for both New Game and Load Game)
    shardCount = SaveData::current.shardCount;
    score      = SaveData::current.score;

    player->SetHealth  (SaveData::current.health);
    player->SetLives   (SaveData::current.lives);
    player->SetPosition(SaveData::current.posX, SaveData::current.posY);

    bot01->SetTotalTime(SaveData::current.waveTime);

    // Shard beacon — satellite marker placed at death position
    shardBeacon = new ShardBeacon();
    if (!shardBeacon->OnCreate()) return false;
    if (SaveData::current.hasLostShards) {
        if (SaveData::current.deathLevelTime > 0.0f) {
            // Beacon is pending — activate when level timeline reaches the death moment
            beaconTriggerTime = SaveData::current.deathLevelTime;
        } else {
            // Beacon was already triggered last session — restore immediately
            shardBeacon->Place(
                Vec3(SaveData::current.lostShardPosX,
                     SaveData::current.lostShardPosY, -10.0f),
                SaveData::current.lostShardCount);
        }
    }

    prevLives = player->GetLives();

    // Apply saved audio preferences
    SDL_SetAudioStreamGain(bgmPlayer, SaveData::current.musicVolume);
    SDL_SetAudioStreamGain(sfxPlayer,   SaveData::current.sfxVolume);
    musicVolume = SaveData::current.musicVolume;
    sfxVolume   = SaveData::current.sfxVolume;

    // Level director — loads all environment chunk meshes up front, no runtime stutter
    levelDirector = new LevelDirector();
    levelDirector->SetPhaseCallback([this](int id) { currentPhase = id; });
    levelDirector->SetBot01Callback([this](int count, float interval, bool shielded) {
        bot01->TriggerWave(shielded ? Bot01WaveType::SHIELDED : Bot01WaveType::STANDARD,
                           count, interval);
    });
    levelDirector->SetBot02Callback([this]() {
        if (bot02->GetCount() == 0)
            bot02->Spawn(player->GetPosition().y);
    });
    levelDirector->SetAsteroidCallback([this](float largeInterval, float smallInterval) {
        asteroid->SetSpawnRates(largeInterval, smallInterval);
    });
    levelDirector->AddScript(new Level01Script(), 0.0f);
    // Level02 starts at t=180s. Bring offset down to ~158s for a 22s overlap window
    // where both zones' chunks scroll on screen simultaneously (seamless transition).
    levelDirector->AddScript(new Level02Script(), 180.0f);

    debugOverlay = new DebugOverlay();

    return true;
}

void SceneMuntasir::OnVideoChanged(int w, int h) {
    float aspect = static_cast<float>(w) / static_cast<float>(h);
    GameConst::ComputeWorldBounds(aspect);
    projectionMatrix = MMath::perspective(70.0f, aspect, 0.1f, 100.0f);
    environment->OnResize((float)w, (float)h);
}

// OnDestroy
void SceneMuntasir::OnDestroy() {
    Debug::Info("Deleting assets SceneMuntasir: ", __FILE__, __LINE__);

    // On a complete death: reset only run-specific fields so the next load starts fresh.
    // highScore was committed at game-over detection; shard pile was saved at last life-loss.
    // On a normal mid-run exit: full mid-session save so the player can resume.
    if (gameOver) {
        SaveData::current.lives       = 3;
        SaveData::current.health      = 100.0f;
        SaveData::current.score       = 0;
        SaveData::current.shardCount  = 0;
        SaveData::current.posX        = 0.0f;
        SaveData::current.posY        = 0.0f;
        SaveData::current.waveTime    = 0.0f;
        SaveData::current.Save();
    } else {
        SaveGame();
    }

    // Level director
    if (levelDirector) {
        levelDirector->OnDestroy();
        delete levelDirector;
        levelDirector = nullptr;
    }

    // Shards
    shards.clear();
    if (shardMesh) {
        shardMesh->OnDestroy();
        delete shardMesh;
        shardMesh = nullptr;
    }
    if (shardBeacon) {
        shardBeacon->OnDestroy();
        delete shardBeacon;
        shardBeacon = nullptr;
    }

    // Game classes
    player->OnDestroy();
    delete player;
    player = nullptr;

    asteroid->OnDestroy();
    delete asteroid;
    asteroid = nullptr;

    bot01->OnDestroy();
    delete bot01;
    bot01 = nullptr;

    bot02->OnDestroy();
    delete bot02;
    bot02 = nullptr;

    bullet->OnDestroy();
    delete bullet;
    bullet = nullptr;

    environment->OnDestroy();
    delete environment;
    environment = nullptr;

    // Shader
    shader->OnDestroy();
    delete shader;

    // SFX
    sfxLaser->OnDestroy();
    delete sfxLaser;

    sfxLaserHit->OnDestroy();
    delete sfxLaserHit;

    sfxExplosion->OnDestroy();
    delete sfxExplosion;

    sfxMissileHit->OnDestroy();
    delete sfxMissileHit;

    sfxShieldHit->OnDestroy();
    delete sfxShieldHit;

    if (uiClickSound) {
        uiClickSound->OnDestroy();
        delete uiClickSound;
        uiClickSound = nullptr;
    }
    if (hoverStream) {
        SDL_DestroyAudioStream(hoverStream);
        hoverStream = nullptr;
    }

    // Audio
    SDL_DestroyAudioStream(bgmPlayer);
    SDL_DestroyAudioStream(sfxPlayer);
    SDL_DestroyAudioStream(sfxLaserHitStream);
    bgmMusic->OnDestroy();
    delete bgmMusic;

    delete debugOverlay;
    debugOverlay = nullptr;
}

// HandleEvents
void SceneMuntasir::HandleEvents(const SDL_Event& sdlEvent) {
    switch (sdlEvent.type) {
    case SDL_EVENT_KEY_DOWN:
        switch (sdlEvent.key.scancode) {
        case SDL_SCANCODE_ESCAPE:
            if (!gameOver) gamePaused = !gamePaused;
            break;
        case SDL_SCANCODE_F9:
            showDebugOverlay = !showDebugOverlay;
            break;
        case SDL_SCANCODE_F12:
            drawInWireMode = !drawInWireMode;
            break;
        case SDL_SCANCODE_SPACE:
            bullet->Spawn(player->GetPosition());
            player->ApplyImpulse(Vec3(-1.5f, 0.0f, 0.0f)); // recoil
            sfxLaser->Play(sfxPlayer);
            break;
        default:
            player->HandleEvents(sdlEvent);
            break;
        }
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (sdlEvent.button.button == SDL_BUTTON_LEFT) {
            if (!ImGui::GetIO().WantCaptureMouse) {
                bullet->Spawn(player->GetPosition());
                player->ApplyImpulse(Vec3(-1.5f, 0.0f, 0.0f)); // recoil
                sfxLaser->Play(sfxPlayer);
            }
        }
        // Right click - homing missile
        if (sdlEvent.button.button == SDL_BUTTON_RIGHT) {
            if (!ImGui::GetIO().WantCaptureMouse && bullet->GetMissileCount() > 0) {
                auto nearestIdx = [&](const std::vector<Vec3>& pool) -> int {
                    Vec3 p = player->GetPosition();
                    int best = 0;
                    float bestDist = -1.0f;
                    for (int i = 0; i < (int)pool.size(); i++) {
                        float dx = pool[i].x - p.x, dy = pool[i].y - p.y;
                        float d = dx*dx + dy*dy;
                        if (bestDist < 0.0f || d < bestDist) { bestDist = d; best = i; }
                    }
                    return best;
                };

                bool launched = false;
                if (!bot02->GetPositions().empty()) {
                    bullet->SpawnHoming(player->GetPosition(),
                        MissileTargetType::BOT02, nearestIdx(bot02->GetPositions()));
                    launched = true;
                }
                else if (!bot01->GetBot01Positions().empty()) {
                    bullet->SpawnHoming(player->GetPosition(),
                        MissileTargetType::BOT01, nearestIdx(bot01->GetBot01Positions()));
                    launched = true;
                }
                else if (!asteroid->GetAsteroidPositions().empty()) {
                    bullet->SpawnHoming(player->GetPosition(),
                        MissileTargetType::ASTEROID, nearestIdx(asteroid->GetAsteroidPositions()));
                    launched = true;
                }
                if (launched)
                    player->ApplyImpulse(Vec3(-3.5f, 0.0f, 0.0f)); // heavier recoil than laser
            }
        }
        break;
    }
}

// Pack current full game state into SaveData::current then write to disk
void SceneMuntasir::SaveGame() {
    SaveData::current.shardCount      = shardCount;
    SaveData::current.score           = score;
    SaveData::current.health          = player->GetHealth();
    SaveData::current.lives           = player->GetLives();
    SaveData::current.posX            = player->GetPosition().x;
    SaveData::current.posY            = player->GetPosition().y;
    SaveData::current.waveTime        = bot01->GetTotalTime();
    if (shardBeacon->IsActive()) {
        // Beacon is visible — sync from beacon object; reload should show it immediately
        SaveData::current.hasLostShards  = true;
        SaveData::current.lostShardPosX  = shardBeacon->GetPosition().x;
        SaveData::current.lostShardPosY  = shardBeacon->GetPosition().y;
        SaveData::current.lostShardCount = shardBeacon->GetCount();
        SaveData::current.deathLevelTime = 0.0f;
    } else if (beaconTriggerTime > 0.0f) {
        // Beacon is pending — level hasn't reached the death time yet; keep pos/count as-is
        SaveData::current.hasLostShards  = true;
        SaveData::current.deathLevelTime = beaconTriggerTime;
    } else {
        SaveData::current.hasLostShards  = false;
        SaveData::current.lostShardPosX  = 0.0f;
        SaveData::current.lostShardPosY  = 0.0f;
        SaveData::current.lostShardCount = 0;
        SaveData::current.deathLevelTime = 0.0f;
    }
    SaveData::current.musicVolume     = musicVolume;
    SaveData::current.sfxVolume       = sfxVolume;
    SaveData::current.Save();
}

void SceneMuntasir::SpawnShards(const Vec3& pos, int count) {
    for (int i = 0; i < count; i++) {
        Shard s;
        s.pos = pos;
        float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
        float spd   = 0.3f + (rand() % 50) * 0.01f;   // 0.30 .. 0.80 units/s drift
        s.vel       = Vec3(cosf(angle) * spd, sinf(angle) * spd, 0.0f);
        s.angle     = (float)(rand() % 360);
        s.spinSpeed = (float)((rand() % 241) - 120);    // -120..+120 deg/s
        shards.push_back(s);
    }
}

// Update
void SceneMuntasir::Update(const float deltaTime) {
    debugOverlay->Update(deltaTime);

    if (gamePaused) return;

    // Cooldown timers
    if (explosionCooldownTimer > 0.0f)
        explosionCooldownTimer -= deltaTime;
    if (shieldHitCooldownTimer > 0.0f)
        shieldHitCooldownTimer -= deltaTime;

    // Periodic auto-save (only while alive)
    if (!gameOver) {
        autoSaveTimer += deltaTime;
        if (autoSaveTimer >= kAutoSaveInterval) {
            autoSaveTimer = 0.0f;
            SaveGame();
        }
    }

    if (gameOver) return;

    // Advance level timeline and scroll environment chunks
    levelDirector->Update(deltaTime);

    // Update all classes
    player->Update(deltaTime);
    bullet->Update(deltaTime,
        asteroid->GetAsteroidPositions(), Vec3(-asteroid->GetAsteroidSpeed(), 0.0f, 0.0f),
        bot01->GetBot01Positions(),       Vec3(-bot01->GetBot01Speed(), 0.0f, 0.0f),
        bot02->GetPositions(),            Vec3(0.0f, 0.0f, 0.0f));
    asteroid->Update(deltaTime);
    bot01->Update(deltaTime, player->GetPosition().x, player->GetPosition().y);
    bot01->UpdateShields(deltaTime, bullet->GetMissilePositions());
    bot02->Update(deltaTime, player->GetPosition().x, player->GetPosition().y);
    environment->Update(deltaTime);

    // Shard physics — drift, magnet pull, collect, cull
    Vec3 ppos = player->GetPosition();
    for (int i = (int)shards.size() - 1; i >= 0; i--) {
        // Drift
        shards[i].pos   += shards[i].vel * deltaTime;
        shards[i].angle += shards[i].spinSpeed * deltaTime;

        // Magnet attachment pull (inverse-linear: closer = stronger)
        float dx   = ppos.x - shards[i].pos.x;
        float dy   = ppos.y - shards[i].pos.y;
        float dist = sqrtf(dx*dx + dy*dy);
        if (dist < kMagnetRadius && dist > 0.01f) {
            float pull = 5.0f / dist;
            shards[i].vel.x += (dx / dist) * pull * deltaTime;
            shards[i].vel.y += (dy / dist) * pull * deltaTime;
            // Clamp so they don't rocket past the ship
            float vspd = sqrtf(shards[i].vel.x * shards[i].vel.x +
                               shards[i].vel.y * shards[i].vel.y);
            if (vspd > 6.0f) {
                shards[i].vel.x = shards[i].vel.x / vspd * 6.0f;
                shards[i].vel.y = shards[i].vel.y / vspd * 6.0f;
            }
        }

        // Collect
        if (dx*dx + dy*dy < kCollectRadius * kCollectRadius) {
            shardCount++;
            shards.erase(shards.begin() + i);
            continue;
        }

        // Cull off-screen
        if (shards[i].pos.x < GameConst::kCullX || shards[i].pos.x > GameConst::kSpawnX ||
            shards[i].pos.y < -10.0f || shards[i].pos.y >  10.0f) {
            shards.erase(shards.begin() + i);
        }
    }

    // Shard beacon — activate when level timeline reaches recorded death moment
    if (!shardBeacon->IsActive() && beaconTriggerTime > 0.0f &&
        levelDirector->GetTime() >= beaconTriggerTime) {
        shardBeacon->Place(
            Vec3(SaveData::current.lostShardPosX,
                 SaveData::current.lostShardPosY, -10.0f),
            SaveData::current.lostShardCount);
        beaconTriggerTime = 0.0f;
    }

    // Update animation, then check pickup
    shardBeacon->Update(deltaTime);
    int recovered = shardBeacon->TryCollect(ppos);
    if (recovered > 0) {
        shardCount += recovered;
        SaveData::current.hasLostShards  = false;
        SaveData::current.lostShardPosX  = 0.0f;
        SaveData::current.lostShardPosY  = 0.0f;
        SaveData::current.lostShardCount = 0;
        SaveData::current.deathLevelTime = 0.0f;
        beaconTriggerTime = 0.0f;
        SaveGame();
    }

    // Check game over
    if (player->IsGameOver()) {
        if (score > SaveData::current.highScore)
            SaveData::current.highScore = score;
        if (shardCount > 0) {
            // Drop ALL shards at final death position — replaces any existing beacon
            shardBeacon->Clear();
            SaveData::current.hasLostShards  = true;
            SaveData::current.lostShardPosX  = player->GetPosition().x;
            SaveData::current.lostShardPosY  = player->GetPosition().y;
            SaveData::current.lostShardCount = shardCount;
            SaveData::current.deathLevelTime = levelDirector->GetTime();
            SaveData::current.shardCount     = 0;
            beaconTriggerTime = SaveData::current.deathLevelTime;
            shardCount = 0;
        }
        // shardCount == 0: any existing beacon from a prior run survives untouched
        gameOver = true;
    }

    // Ellipse helper: (dx/rx)^2 + (dy/ry)^2 < 1
    // rx = front/back extent, ry = top/bottom extent

    // Bullet hits Bot01
    for (int b = bullet->GetPositions().size() - 1; b >= 0; b--) {
        for (int e = bot01->GetBot01Positions().size() - 1; e >= 0; e--) {
            float dx = bullet->GetPositions()[b].x - bot01->GetBot01Positions()[e].x;
            float dy = bullet->GetPositions()[b].y - bot01->GetBot01Positions()[e].y;
            if ((dx*dx)/(0.6f*0.6f) + (dy*dy)/(0.32f*0.32f) < 1.0f) {
                Vec3 deathPos = bot01->GetBot01Positions()[e];
                bullet->RemoveAt(b);
                bot01->PushX(e, 1.0f);
                bot01->PushY(e, 0.0f);
                SDL_ClearAudioStream(sfxLaserHitStream);
                sfxLaserHit->Play(sfxLaserHitStream);
                if (bot01->DamageBot01(e)) {
                    SpawnShards(deathPos, 5);
                    score += 100;
                    if (explosionCooldownTimer <= 0.0f) {
                        sfxExplosion->Play(sfxPlayer);
                        explosionCooldownTimer = explosionCooldown;
                    }
                }
                break;
            }
        }
    }

    // Bullet hits large asteroid
    for (int b = bullet->GetPositions().size() - 1; b >= 0; b--) {
        for (int a = asteroid->GetAsteroidPositions().size() - 1; a >= 0; a--) {
            float dx = bullet->GetPositions()[b].x - asteroid->GetAsteroidPositions()[a].x;
            float dy = bullet->GetPositions()[b].y - asteroid->GetAsteroidPositions()[a].y;
            if ((dx*dx)/(0.85f*0.85f) + (dy*dy)/(0.7f*0.7f) < 1.0f) {
                Vec3 deathPos = asteroid->GetAsteroidPositions()[a];
                bullet->RemoveAt(b);
                asteroid->PushAsteroid(a, 1.5f, 0.0f);
                SDL_ClearAudioStream(sfxLaserHitStream);
                sfxLaserHit->Play(sfxLaserHitStream);
                if (asteroid->DamageAsteroid(a)) {
                    SpawnShards(deathPos, 3);
                    score += 50;
                    if (explosionCooldownTimer <= 0.0f) {
                        sfxExplosion->Play(sfxPlayer);
                        explosionCooldownTimer = explosionCooldown;
                    }
                }
                break;
            }
        }
    }

    // Bullet hits small asteroid
    for (int b = bullet->GetPositions().size() - 1; b >= 0; b--) {
        for (int a = asteroid->GetSmallAsteroidPositions().size() - 1; a >= 0; a--) {
            float dx = bullet->GetPositions()[b].x - asteroid->GetSmallAsteroidPositions()[a].x;
            float dy = bullet->GetPositions()[b].y - asteroid->GetSmallAsteroidPositions()[a].y;
            if ((dx*dx)/(0.45f*0.45f) + (dy*dy)/(0.38f*0.38f) < 1.0f) {
                Vec3 deathPos = asteroid->GetSmallAsteroidPositions()[a];
                bullet->RemoveAt(b);
                asteroid->PushSmallAsteroid(a, 2.5f, 0.0f);
                SDL_ClearAudioStream(sfxLaserHitStream);
                sfxLaserHit->Play(sfxLaserHitStream);
                if (asteroid->DamageSmallAsteroid(a)) {
                    SpawnShards(deathPos, 2);
                    score += 25;
                    if (explosionCooldownTimer <= 0.0f) {
                        sfxExplosion->Play(sfxPlayer);
                        explosionCooldownTimer = explosionCooldown;
                    }
                }
                break;
            }
        }
    }

    // Missile hits Bot01 — 4x damage, knockback, SFX+shards on every hit
    // Shielded bots absorb the missile without taking damage (shield blocks it)
    for (int m = (int)bullet->GetMissilePositions().size() - 1; m >= 0; m--) {
        for (int e = (int)bot01->GetBot01Positions().size() - 1; e >= 0; e--) {
            float dx = bullet->GetMissilePositions()[m].x - bot01->GetBot01Positions()[e].x;
            float dy = bullet->GetMissilePositions()[m].y - bot01->GetBot01Positions()[e].y;
            if ((dx*dx)/(0.65f*0.65f) + (dy*dy)/(0.35f*0.35f) < 1.0f) {
                if (bot01->IsBot01ShieldActive(e)) {
                    bullet->RemoveMissileAt(m);
                    break;
                }
                Vec3 deathPos = bot01->GetBot01Positions()[e];
                // Derive impact direction from missile velocity at moment of hit.
                // normalize(vel) gives the true 3D approach angle — diagonal curve
                // hits split force across X and Y naturally (industry standard).
                Vec3  mVel   = bullet->GetMissileVelocities()[m];
                float mSpd   = sqrtf(mVel.x*mVel.x + mVel.y*mVel.y);
                float iDirX  = (mSpd > 0.001f) ? mVel.x / mSpd : 1.0f;
                float iDirY  = (mSpd > 0.001f) ? mVel.y / mSpd : 0.0f;
                bot01->PushX(e, iDirX * 3.0f); // backward push proportional to X angle
                bot01->PushY(e, iDirY * 5.5f); // Y push proportional to actual curve angle
                bullet->RemoveMissileAt(m);
                if (bot01->DamageBot01(e, 4)) {
                    SpawnShards(deathPos, 7);
                    score += 100;
                    if (explosionCooldownTimer <= 0.0f) {
                        sfxExplosion->Play(sfxPlayer);
                        explosionCooldownTimer = explosionCooldown;
                    }
                } else {
                    SpawnShards(deathPos, 2);
                    sfxMissileHit->Play(sfxPlayer);
                }
                break;
            }
        }
    }

    // Missile hits large asteroid — 3x damage, SFX+shards on every hit
    for (int m = (int)bullet->GetMissilePositions().size() - 1; m >= 0; m--) {
        for (int a = (int)asteroid->GetAsteroidPositions().size() - 1; a >= 0; a--) {
            float dx = bullet->GetMissilePositions()[m].x - asteroid->GetAsteroidPositions()[a].x;
            float dy = bullet->GetMissilePositions()[m].y - asteroid->GetAsteroidPositions()[a].y;
            if ((dx*dx)/(0.9f*0.9f) + (dy*dy)/(0.75f*0.75f) < 1.0f) {
                Vec3 deathPos = asteroid->GetAsteroidPositions()[a];
                Vec3  mVelA  = bullet->GetMissileVelocities()[m];
                float mSpdA  = sqrtf(mVelA.x*mVelA.x + mVelA.y*mVelA.y);
                float iDirAX = (mSpdA > 0.001f) ? mVelA.x / mSpdA : 1.0f;
                float iDirAY = (mSpdA > 0.001f) ? mVelA.y / mSpdA : 0.0f;
                asteroid->PushAsteroid(a, iDirAX * 2.5f, iDirAY * 1.5f);
                bullet->RemoveMissileAt(m);
                if (asteroid->DamageAsteroid(a, 3)) {
                    SpawnShards(deathPos, 5);
                    score += 50;
                    if (explosionCooldownTimer <= 0.0f) {
                        sfxExplosion->Play(sfxPlayer);
                        explosionCooldownTimer = explosionCooldown;
                    }
                } else {
                    SpawnShards(deathPos, 2);
                    sfxMissileHit->Play(sfxPlayer);
                }
                break;
            }
        }
    }

    // Missile hits small asteroid — 3x damage (instant kill), SFX+shards
    for (int m = (int)bullet->GetMissilePositions().size() - 1; m >= 0; m--) {
        for (int a = (int)asteroid->GetSmallAsteroidPositions().size() - 1; a >= 0; a--) {
            float dx = bullet->GetMissilePositions()[m].x - asteroid->GetSmallAsteroidPositions()[a].x;
            float dy = bullet->GetMissilePositions()[m].y - asteroid->GetSmallAsteroidPositions()[a].y;
            if ((dx*dx)/(0.5f*0.5f) + (dy*dy)/(0.4f*0.4f) < 1.0f) {
                Vec3 deathPos = asteroid->GetSmallAsteroidPositions()[a];
                Vec3  mVelS  = bullet->GetMissileVelocities()[m];
                float mSpdS  = sqrtf(mVelS.x*mVelS.x + mVelS.y*mVelS.y);
                float iDirSX = (mSpdS > 0.001f) ? mVelS.x / mSpdS : 1.0f;
                float iDirSY = (mSpdS > 0.001f) ? mVelS.y / mSpdS : 0.0f;
                asteroid->PushSmallAsteroid(a, iDirSX * 4.0f, iDirSY * 2.5f);
                bullet->RemoveMissileAt(m);
                if (asteroid->DamageSmallAsteroid(a, 3)) {
                    SpawnShards(deathPos, 3);
                    score += 25;
                    if (explosionCooldownTimer <= 0.0f) {
                        sfxExplosion->Play(sfxPlayer);
                        explosionCooldownTimer = explosionCooldown;
                    }
                } else {
                    SpawnShards(deathPos, 1);
                    sfxMissileHit->Play(sfxPlayer);
                }
                break;
            }
        }
    }

    // === Shield bubble collision — anything entering the ellipse is destroyed ===
    // Ellipse shape matches the rendered mesh exactly: X half-axis 1.05, Y 0.75 world units.
    if (player->IsShieldActive()) {
        const Vec3  sp  = player->GetPosition();
        const float srx = player->GetShieldRadiusX();
        const float sry = player->GetShieldRadiusY();
        const float srx2 = srx * srx;
        const float sry2 = sry * sry;

        // Large asteroids
        for (int a = (int)asteroid->GetAsteroidPositions().size() - 1; a >= 0; a--) {
            float dx = asteroid->GetAsteroidPositions()[a].x - sp.x;
            float dy = asteroid->GetAsteroidPositions()[a].y - sp.y;
            if ((dx*dx)/srx2 + (dy*dy)/sry2 < 1.0f) {
                Vec3 pos = asteroid->GetAsteroidPositions()[a];
                if (asteroid->DamageAsteroid(a, 999)) {
                    SpawnShards(pos, 3);
                    score += 10;
                    if (explosionCooldownTimer <= 0.0f) {
                        sfxExplosion->Play(sfxPlayer);
                        explosionCooldownTimer = explosionCooldown;
                    }
                }
            }
        }
        // Small asteroids
        for (int a = (int)asteroid->GetSmallAsteroidPositions().size() - 1; a >= 0; a--) {
            float dx = asteroid->GetSmallAsteroidPositions()[a].x - sp.x;
            float dy = asteroid->GetSmallAsteroidPositions()[a].y - sp.y;
            if ((dx*dx)/srx2 + (dy*dy)/sry2 < 1.0f) {
                Vec3 pos = asteroid->GetSmallAsteroidPositions()[a];
                if (asteroid->DamageSmallAsteroid(a, 999)) {
                    SpawnShards(pos, 2);
                    score += 5;
                    if (explosionCooldownTimer <= 0.0f) {
                        sfxExplosion->Play(sfxPlayer);
                        explosionCooldownTimer = explosionCooldown;
                    }
                }
            }
        }
        // Bot01
        for (int e = (int)bot01->GetBot01Positions().size() - 1; e >= 0; e--) {
            float dx = bot01->GetBot01Positions()[e].x - sp.x;
            float dy = bot01->GetBot01Positions()[e].y - sp.y;
            if ((dx*dx)/srx2 + (dy*dy)/sry2 < 1.0f) {
                Vec3 pos = bot01->GetBot01Positions()[e];
                if (bot01->DamageBot01(e, 999)) {
                    SpawnShards(pos, 5);
                    score += 50;
                    if (explosionCooldownTimer <= 0.0f) {
                        sfxExplosion->Play(sfxPlayer);
                        explosionCooldownTimer = explosionCooldown;
                    }
                }
            }
        }
        // Bot02 bullets — moving-reflector ricochet.
        //
        // Why bullets always returned to Bot02 before: Bot02 fires straight at the
        // player, so the incoming direction is antiparallel to the ellipse normal at
        // contact — pure elastic reflection sends it exactly back. Adding the shield's
        // own velocity (player velocity) breaks that symmetry: the reflection is
        // computed in the shield's moving reference frame, so lateral movement "spins"
        // the outgoing angle, making deflection skill-based rather than automatic.
        //
        // Steps (moving-wall / pool-cue physics):
        //   1. Express bullet velocity relative to the moving shield: v_rel = v - v_shield
        //   2. Reflect v_rel elastically off the ellipse normal
        //   3. Add the shield velocity back to return to world frame
        //   4. Scale v_shield contribution by kVelInfluence so even slow movement
        //      visibly steers the shot without feeling random at high speed
        //   5. Normalize and lock to kRicochetSpeed for consistent punch
        static constexpr float kRicochetSpeed  = 7.5f;
        static constexpr float kVelInfluence   = 0.3f; // how much shield motion steers the shot
        const Vec3 pVel = player->GetVelocity();
        for (int b = (int)bot02->GetBulletPositions().size() - 1; b >= 0; b--) {
            Vec3& bPos = bot02->GetBulletPositions()[b];
            Vec3& bVel = bot02->GetBulletVelocities()[b];
            float rx = bPos.x - sp.x;
            float ry = bPos.y - sp.y;
            if ((rx*rx)/srx2 + (ry*ry)/sry2 < 1.0f) {
                // Ellipse outward normal: gradient of x²/a² + y²/b²
                float nx = rx / srx2;
                float ny = ry / sry2;
                float nlen = sqrtf(nx*nx + ny*ny);
                if (nlen > 0.0001f) { nx /= nlen; ny /= nlen; }

                // Bullet velocity in shield's reference frame
                float rvx = bVel.x - pVel.x;
                float rvy = bVel.y - pVel.y;
                float relDotN = rvx * nx + rvy * ny;

                if (relDotN < 0.0f) { // moving inward relative to shield — valid bounce
                    // Elastic reflection in shield frame
                    rvx -= 2.0f * relDotN * nx;
                    rvy -= 2.0f * relDotN * ny;

                    // Back to world frame, with scaled player velocity contribution
                    float outVx = rvx + pVel.x * kVelInfluence;
                    float outVy = rvy + pVel.y * kVelInfluence;

                    // Normalize and set fixed punch speed
                    float spd = sqrtf(outVx*outVx + outVy*outVy);
                    if (spd > 0.001f) {
                        bVel.x = outVx / spd * kRicochetSpeed;
                        bVel.y = outVy / spd * kRicochetSpeed;
                    }
                    bot02->MarkBulletReflected(b);

                    if (shieldHitCooldownTimer <= 0.0f) {
                        sfxShieldHit->Play(sfxPlayer);
                        shieldHitCooldownTimer = kShieldHitCooldown;
                    }
                }
            }
        }
    }

    // Reflected Bot02 bullets hit any destructible — consumed on first hit
    for (int b = (int)bot02->GetBulletPositions().size() - 1; b >= 0; b--) {
        if (!bot02->IsBulletReflected(b)) continue;
        bool hit = false;

        // Bot02 — 2 damage
        for (int e = (int)bot02->GetPositions().size() - 1; e >= 0 && !hit; e--) {
            float dx = bot02->GetBulletPositions()[b].x - bot02->GetPositions()[e].x;
            float dy = bot02->GetBulletPositions()[b].y - bot02->GetPositions()[e].y;
            if ((dx*dx)/(0.75f*0.75f) + (dy*dy)/(0.4f*0.4f) < 1.0f) {
                Vec3 deathPos = bot02->GetPositions()[e];
                bot02->RemoveBullet(b);
                if (bot02->DamageBot02(e, 2)) {
                    SpawnShards(deathPos, 8);
                    score += 300;
                    if (explosionCooldownTimer <= 0.0f) { sfxExplosion->Play(sfxPlayer); explosionCooldownTimer = explosionCooldown; }
                }
                hit = true;
            }
        }

        // Bot01 — 2 damage
        for (int e = (int)bot01->GetBot01Positions().size() - 1; e >= 0 && !hit; e--) {
            float dx = bot02->GetBulletPositions()[b].x - bot01->GetBot01Positions()[e].x;
            float dy = bot02->GetBulletPositions()[b].y - bot01->GetBot01Positions()[e].y;
            if ((dx*dx)/(0.65f*0.65f) + (dy*dy)/(0.35f*0.35f) < 1.0f) {
                Vec3 deathPos = bot01->GetBot01Positions()[e];
                bot02->RemoveBullet(b);
                if (bot01->DamageBot01(e, 2)) {
                    SpawnShards(deathPos, 5);
                    score += 100;
                    if (explosionCooldownTimer <= 0.0f) { sfxExplosion->Play(sfxPlayer); explosionCooldownTimer = explosionCooldown; }
                }
                hit = true;
            }
        }

        // Large asteroid — 3 damage
        for (int a = (int)asteroid->GetAsteroidPositions().size() - 1; a >= 0 && !hit; a--) {
            float dx = bot02->GetBulletPositions()[b].x - asteroid->GetAsteroidPositions()[a].x;
            float dy = bot02->GetBulletPositions()[b].y - asteroid->GetAsteroidPositions()[a].y;
            if ((dx*dx)/(0.9f*0.9f) + (dy*dy)/(0.75f*0.75f) < 1.0f) {
                Vec3 deathPos = asteroid->GetAsteroidPositions()[a];
                bot02->RemoveBullet(b);
                if (asteroid->DamageAsteroid(a, 3)) {
                    SpawnShards(deathPos, 3);
                    score += 50;
                    if (explosionCooldownTimer <= 0.0f) { sfxExplosion->Play(sfxPlayer); explosionCooldownTimer = explosionCooldown; }
                }
                hit = true;
            }
        }

        // Small asteroid — 3 damage (instant kill)
        for (int a = (int)asteroid->GetSmallAsteroidPositions().size() - 1; a >= 0 && !hit; a--) {
            float dx = bot02->GetBulletPositions()[b].x - asteroid->GetSmallAsteroidPositions()[a].x;
            float dy = bot02->GetBulletPositions()[b].y - asteroid->GetSmallAsteroidPositions()[a].y;
            if ((dx*dx)/(0.5f*0.5f) + (dy*dy)/(0.4f*0.4f) < 1.0f) {
                Vec3 deathPos = asteroid->GetSmallAsteroidPositions()[a];
                bot02->RemoveBullet(b);
                if (asteroid->DamageSmallAsteroid(a, 3)) {
                    SpawnShards(deathPos, 2);
                    score += 25;
                    if (explosionCooldownTimer <= 0.0f) { sfxExplosion->Play(sfxPlayer); explosionCooldownTimer = explosionCooldown; }
                }
                hit = true;
            }
        }
    }

    // Asteroid hits player
    for (int a = asteroid->GetAsteroidPositions().size() - 1; a >= 0; a--) {
        float dx = player->GetPosition().x - asteroid->GetAsteroidPositions()[a].x;
        float dy = player->GetPosition().y - asteroid->GetAsteroidPositions()[a].y;
        if ((dx*dx)/(1.0f*1.0f) + (dy*dy)/(0.5f*0.5f) < 1.0f) {
            Vec3 deathPos = asteroid->GetAsteroidPositions()[a];
            float len = sqrtf(dx*dx + dy*dy);
            if (len > 0.001f) player->ApplyImpulse(Vec3(dx/len * 4.0f, dy/len * 4.0f, 0.0f));
            asteroid->RemoveAsteroid(a);
            SpawnShards(deathPos, 3);
            player->TakeDamage(25.0f);
            if (explosionCooldownTimer <= 0.0f) {
                sfxExplosion->Play(sfxPlayer);
                explosionCooldownTimer = explosionCooldown;
            }
        }
    }

    // Small asteroid hits player
    for (int a = asteroid->GetSmallAsteroidPositions().size() - 1; a >= 0; a--) {
        float dx = player->GetPosition().x - asteroid->GetSmallAsteroidPositions()[a].x;
        float dy = player->GetPosition().y - asteroid->GetSmallAsteroidPositions()[a].y;
        if ((dx*dx)/(0.65f*0.65f) + (dy*dy)/(0.35f*0.35f) < 1.0f) {
            Vec3 deathPos = asteroid->GetSmallAsteroidPositions()[a];
            float len = sqrtf(dx*dx + dy*dy);
            if (len > 0.001f) player->ApplyImpulse(Vec3(dx/len * 2.5f, dy/len * 2.5f, 0.0f));
            asteroid->RemoveSmallAsteroid(a);
            SpawnShards(deathPos, 2);
            player->TakeDamage(10.0f);
            if (explosionCooldownTimer <= 0.0f) {
                sfxExplosion->Play(sfxPlayer);
                explosionCooldownTimer = explosionCooldown;
            }
        }
    }

    // Bot01 hits player
    for (int e = bot01->GetBot01Positions().size() - 1; e >= 0; e--) {
        float dx = player->GetPosition().x - bot01->GetBot01Positions()[e].x;
        float dy = player->GetPosition().y - bot01->GetBot01Positions()[e].y;
        if ((dx*dx)/(0.75f*0.75f) + (dy*dy)/(0.4f*0.4f) < 1.0f) {
            Vec3 deathPos = bot01->GetBot01Positions()[e];
            float len = sqrtf(dx*dx + dy*dy);
            if (len > 0.001f) player->ApplyImpulse(Vec3(dx/len * 5.0f, dy/len * 5.0f, 0.0f));
            bot01->RemoveBot01(e);
            SpawnShards(deathPos, 5);
            player->TakeDamage(40.0f);
            if (explosionCooldownTimer <= 0.0f) {
                sfxExplosion->Play(sfxPlayer);
                explosionCooldownTimer = explosionCooldown;
            }
        }
    }

    // Bullet hits Bot02
    for (int b = bullet->GetPositions().size() - 1; b >= 0; b--) {
        for (int e = bot02->GetPositions().size() - 1; e >= 0; e--) {
            float dx = bullet->GetPositions()[b].x - bot02->GetPositions()[e].x;
            float dy = bullet->GetPositions()[b].y - bot02->GetPositions()[e].y;
            if ((dx*dx)/(0.7f*0.7f) + (dy*dy)/(0.35f*0.35f) < 1.0f) {
                Vec3 deathPos = bot02->GetPositions()[e];
                bullet->RemoveAt(b);
                bot02->PushBot02(e, 0.5f, 0.0f);
                if (bot02->DamageBot02(e)) {
                    SpawnShards(deathPos, 8);
                    score += 300;
                    if (explosionCooldownTimer <= 0.0f) {
                        sfxExplosion->Play(sfxPlayer);
                        explosionCooldownTimer = explosionCooldown;
                    }
                }
                break;
            }
        }
    }

    // Missile hits Bot02 — 4x damage, SFX+shards on every hit
    for (int m = (int)bullet->GetMissilePositions().size() - 1; m >= 0; m--) {
        for (int e = (int)bot02->GetPositions().size() - 1; e >= 0; e--) {
            float dx = bullet->GetMissilePositions()[m].x - bot02->GetPositions()[e].x;
            float dy = bullet->GetMissilePositions()[m].y - bot02->GetPositions()[e].y;
            if ((dx*dx)/(0.75f*0.75f) + (dy*dy)/(0.4f*0.4f) < 1.0f) {
                Vec3 deathPos = bot02->GetPositions()[e];
                Vec3  mVel2  = bullet->GetMissileVelocities()[m];
                float mSpd2  = sqrtf(mVel2.x*mVel2.x + mVel2.y*mVel2.y);
                float iDirX2 = (mSpd2 > 0.001f) ? mVel2.x / mSpd2 : 1.0f;
                float iDirY2 = (mSpd2 > 0.001f) ? mVel2.y / mSpd2 : 0.0f;
                bot02->PushBot02(e, iDirX2 * 2.0f, iDirY2 * 3.5f);
                bullet->RemoveMissileAt(m);
                if (bot02->DamageBot02(e, 4)) {
                    SpawnShards(deathPos, 10);
                    score += 300;
                    if (explosionCooldownTimer <= 0.0f) {
                        sfxExplosion->Play(sfxPlayer);
                        explosionCooldownTimer = explosionCooldown;
                    }
                } else {
                    SpawnShards(deathPos, 3);
                    sfxMissileHit->Play(sfxPlayer);
                }
                break;
            }
        }
    }

    // Bot02 hits player
    for (int e = bot02->GetPositions().size() - 1; e >= 0; e--) {
        float dx = player->GetPosition().x - bot02->GetPositions()[e].x;
        float dy = player->GetPosition().y - bot02->GetPositions()[e].y;
        if ((dx*dx)/(0.8f*0.8f) + (dy*dy)/(0.45f*0.45f) < 1.0f) {
            Vec3 deathPos = bot02->GetPositions()[e];
            float len = sqrtf(dx*dx + dy*dy);
            if (len > 0.001f) player->ApplyImpulse(Vec3(dx/len * 6.0f, dy/len * 6.0f, 0.0f));
            bot02->RemoveBot02(e);
            SpawnShards(deathPos, 8);
            player->TakeDamage(50.0f);
            if (explosionCooldownTimer <= 0.0f) {
                sfxExplosion->Play(sfxPlayer);
                explosionCooldownTimer = explosionCooldown;
            }
        }
    }

    // Bot02 bullet hits player — slow, precise, heavy impact
    for (int b = (int)bot02->GetBulletPositions().size() - 1; b >= 0; b--) {
        float dx = player->GetPosition().x - bot02->GetBulletPositions()[b].x;
        float dy = player->GetPosition().y - bot02->GetBulletPositions()[b].y;
        if (dx*dx + dy*dy < 0.35f * 0.35f) {
            if (player->IsShieldActive()) {
                bot02->RemoveBullet(b);
                if (shieldHitCooldownTimer <= 0.0f) {
                    sfxShieldHit->Play(sfxPlayer);
                    shieldHitCooldownTimer = kShieldHitCooldown;
                }
            } else {
                Vec3 vel = bot02->GetBulletVelocities()[b];
                float vlen = sqrtf(vel.x*vel.x + vel.y*vel.y);
                if (vlen > 0.001f)
                    player->ApplyImpulse(Vec3(vel.x/vlen * -5.0f, vel.y/vlen * -5.0f, 0.0f));
                bot02->RemoveBullet(b);
                player->TakeDamage(25.0f);
                if (explosionCooldownTimer <= 0.0f) {
                    sfxExplosion->Play(sfxPlayer);
                    explosionCooldownTimer = explosionCooldown;
                }
            }
        }
    }

    // All spawning is driven by the level script.
    // The only phase gates remaining are the Bot02 intro window (phase 3) pause flags.
    bool asteroidsSpawning = currentPhase < 3 || currentPhase >= 4;
    bot01->SetSpawningEnabled(currentPhase != 3);
    asteroid->SetSpawningEnabled(asteroidsSpawning);

    // Life-loss detection — after all collision damage this frame.
    int currentLives = player->GetLives();
    if (currentLives < prevLives) {
        // Shards stay on individual life loss — only a complete game over drops them
        SaveGame();
    }
    prevLives = currentLives;
}

// RenderBackground — pure OpenGL nebula drawn before 3D so it can't tint game objects
void SceneMuntasir::RenderBackground() {
    // Clear to black
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Query actual viewport so this isn't hardcoded to 1920x1080
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    int W = vp[2], H = vp[3];

    // Nebula occupies the top 33% of screen.
    // GL y=0 is at bottom, so top-33% maps to GL y = H*0.67 .. H
    int solidBottom = (int)(H * 0.85f); // top 15%: solid blue
    int gradBottom  = (int)(H * 0.67f); // top 33%: gradient starts here

    glEnable(GL_SCISSOR_TEST);

    // Solid blue band (top 15%)
    glScissor(0, solidBottom, W, H - solidBottom);
    glClearColor(10.f/255.f, 30.f/255.f, 120.f/255.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Gradient band (15%–33%): black at bottom, blue at top
    int gradH = solidBottom - gradBottom;
    const int steps = 24;
    for (int i = 0; i < steps; i++) {
        int y0 = gradBottom + (i * gradH) / steps;
        int y1 = gradBottom + ((i + 1) * gradH) / steps;
        float t = (float)i / (steps - 1); // 0=transparent(bottom), 1=solid(top)
        glScissor(0, y0, W, y1 - y0);
        glClearColor(10.f/255.f * t, 30.f/255.f * t, 120.f/255.f * t, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

// Render
void SceneMuntasir::Render() const {
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT); // color already set by RenderBackground

    // Wireframe
    if (drawInWireMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glUseProgram(shader->GetProgram());

    // Light and camera position (sent to frag shader)
    glUniform3f(shader->GetUniformID("lightPos"), 0.0f, 50.0f, 10.0f);
    glUniform3f(shader->GetUniformID("viewPos"), 0.0f, 0.0f, 10.0f);

    // Player — colors set per-part inside Player::Render()
    player->Render(shader, projectionMatrix, viewMatrix);

    // Bullets — YELLOW
    glUniform4f(shader->GetUniformID("color"), 1.0f, 1.0f, 0.0f, 1.0f);
    bullet->Render(shader, projectionMatrix, viewMatrix);

    // Environment chunks — level geometry scrolling through the scene
    levelDirector->Render(shader, projectionMatrix, viewMatrix);

    asteroid->Render(shader, projectionMatrix, viewMatrix);
    bot01->Render(shader, projectionMatrix, viewMatrix);
    bot02->Render(shader, projectionMatrix, viewMatrix);

    // Energy shards — additive emissive spinning orbs
    if (!shards.empty()) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        glUniform1f(shader->GetUniformID("emissive"), 1.0f);
        glUniform4f(shader->GetUniformID("color"), 1.0f, 0.85f, 0.1f, 0.9f);
        for (int i = 0; i < (int)shards.size(); i++) {
            Matrix4 m = MMath::translate(shards[i].pos) *
                        MMath::rotate(shards[i].angle, Vec3(0.0f, 0.0f, 1.0f)) *
                        MMath::scale(0.12f, 0.12f, 0.12f);
            glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
            shardMesh->Render();
        }
        glUniform1f(shader->GetUniformID("emissive"), 0.0f);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    // Shard beacon — satellite marker at death position (manages its own GL state)
    shardBeacon->Render(shader, projectionMatrix, viewMatrix);

    glUseProgram(0);

    // Stars — ImGui draws these on top of 3D (fine; they're small background dots)
    environment->Render();
}

// Plays a quiet click when the cursor first moves onto a new ImGui widget.
void SceneMuntasir::PlayHoverSound() {
    if (!uiClickSound || !hoverStream || !ImGui::IsItemHovered()) return;
    unsigned int id  = ImGui::GetItemID();
    Uint64       now = SDL_GetTicks();
    if (id == lastHoveredId || now - lastHoverTick < 150) return;
    lastHoveredId = id;
    lastHoverTick = now;
    uiClickSound->Play(hoverStream);
}

void SceneMuntasir::DrawGui() {
    if (showDebugOverlay) debugOverlay->Draw();
    DrawHUD();
    DrawPauseMenu();
    DrawGameOver();
}

// ── HUD — always-visible overlay ─────────────────────────────────────────────
void SceneMuntasir::DrawHUD() {

    // ── Game HUD (always visible) ─────────────────────────────────────────
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(280, 210), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.75f);
    ImGui::Begin("##hud", nullptr,
        ImGuiWindowFlags_NoResize   | ImGuiWindowFlags_NoMove        |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar   |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f),
        "PILOT: %s", SaveData::current.profileName.c_str());
    ImGui::Text("SCORE: %-8d   HI: %d", score, SaveData::current.highScore);
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.1f, 1.0f), "SHARDS: %d", shardCount);
    // Missile slots — solid colored bars (green=ready, dark=spent) + vertical reload bar
    ImGui::Text("MISSILES");
    const ImVec2 slotSize(14.0f, 22.0f);
    const float  slotGap  = 3.0f;
    int maxM = bullet->GetMaxMissiles();
    int curM = bullet->GetMissileCount();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (int mi = 0; mi < maxM; mi++) {
        if (mi > 0) ImGui::SameLine(0.0f, slotGap);
        ImVec2 p = ImGui::GetCursorScreenPos();
        bool  ready = (mi < curM);
        ImU32 fill  = ready ? IM_COL32(55, 210, 80, 255) : IM_COL32(28, 28, 28, 220);
        dl->AddRectFilled(p, ImVec2(p.x + slotSize.x, p.y + slotSize.y), fill, 2.0f);
        dl->AddRect(p, ImVec2(p.x + slotSize.x, p.y + slotSize.y), IM_COL32(160, 160, 160, 200), 2.0f);
        ImGui::Dummy(slotSize);
    }
    // Vertical reload bar — fills from the bottom up while a slot reloads
    if (curM < maxM) {
        ImGui::SameLine(0.0f, slotGap * 2.0f);
        float   frac    = bullet->GetReloadFraction();
        ImVec2  p       = ImGui::GetCursorScreenPos();
        ImVec2  barSize(7.0f, slotSize.y);
        float   fillH   = barSize.y * frac;
        dl->AddRectFilled(p, ImVec2(p.x + barSize.x, p.y + barSize.y), IM_COL32(28, 28, 28, 220), 2.0f);
        dl->AddRectFilled(ImVec2(p.x, p.y + barSize.y - fillH),
                          ImVec2(p.x + barSize.x, p.y + barSize.y),
                          IM_COL32(255, 165, 30, 230), 2.0f);
        dl->AddRect(p, ImVec2(p.x + barSize.x, p.y + barSize.y), IM_COL32(160, 160, 160, 200), 2.0f);
        ImGui::Dummy(barSize);
    }

    // Lives
    ImGui::Text("LIVES:");
    for (int i = 0; i < player->GetLives(); i++) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "[*]");
    }

    // Health bar — green / yellow / red
    float hp = player->GetHealth() / 100.0f;
    ImVec4 hpCol = hp > 0.6f ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f)
                 : hp > 0.3f ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f)
                             : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, hpCol);
    ImGui::ProgressBar(hp, ImVec2(250, 18), "");
    ImGui::PopStyleColor();

    // Shield — single continuous charge bar.
    // Bar drains while active, refills when off. Recharge rate depends on peak usage.
    // Orange tick = 80% usage (slow recharge tier).  Red tick = 90% usage (heavy penalty tier).
    {
        float charge = player->GetShieldChargeFraction(); // 1=full, 0=empty

        // Bar colour follows current charge: cyan (safe) → orange (80% zone) → red (90% zone)
        ImVec4 barCol =
            charge > 0.20f ? ImVec4(0.0f, 0.7f, 1.0f, 1.0f) :
            charge > 0.10f ? ImVec4(1.0f, 0.55f, 0.0f, 1.0f) :
                             ImVec4(0.9f, 0.05f, 0.05f, 1.0f);

        if (player->IsShieldActive()) {
            ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "SHIELD ACTIVE  [E]");
        } else if (player->IsShieldRecharging()) {
            ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.75f, 1.0f), "SHIELD RECHARGING");
        } else {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "SHIELD READY  [E]");
        }

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barCol);
        ImGui::ProgressBar(charge, ImVec2(200.0f, 10.0f), "");
        ImGui::PopStyleColor();

        // Threshold ticks — visible whenever the bar isn't fully charged
        if (charge < 1.0f) {
            ImVec2 bMin = ImGui::GetItemRectMin();
            ImVec2 bMax = ImGui::GetItemRectMax();
            float  barW = bMax.x - bMin.x;
            float tick80X = bMin.x + barW * 0.20f;
            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(tick80X, bMin.y - 1.0f), ImVec2(tick80X, bMax.y + 1.0f),
                IM_COL32(255, 150, 0, 220), 2.0f);
            float tick90X = bMin.x + barW * 0.10f;
            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(tick90X, bMin.y - 1.0f), ImVec2(tick90X, bMax.y + 1.0f),
                IM_COL32(255, 30, 0, 230), 2.0f);
        }
    }

    // Lost shard beacon status
    if (beaconTriggerTime > 0.0f && !shardBeacon->IsActive()) {
        float timeLeft = beaconTriggerTime - levelDirector->GetTime();
        ImGui::TextColored(ImVec4(1.0f, 0.70f, 0.10f, 0.85f),
            "BEACON IN %.0fs  [%d shards]",
            timeLeft, SaveData::current.lostShardCount);
    } else if (shardBeacon->IsActive()) {
        ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.05f, 1.0f),
            ">> RECOVER %d LOST SHARDS", shardBeacon->GetCount());
    }

    ImGui::TextDisabled("ESC  Pause");
    ImGui::End();
}

// ── Pause Menu ────────────────────────────────────────────────────────────────
void SceneMuntasir::DrawPauseMenu() {
    if (!gamePaused) return;

    ImGuiIO& io   = ImGui::GetIO();
        float    panW = 380.0f;
        float    panH = pauseShowSettings ? 640.0f : 275.0f;
        ImGui::SetNextWindowPos(
            ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(panW, panH), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.92f);
        ImGui::Begin("##pause", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

        float innerW = ImGui::GetContentRegionAvail().x;
        float btnW   = innerW - 16.0f;
        float btnX   = 8.0f;

        // "PAUSED" — large centred title, same style as title screen
        ImGui::SetWindowFontScale(2.0f);
        ImVec2 tSz = ImGui::CalcTextSize("PAUSED");
        ImGui::SetCursorPosX((innerW - tSz.x) * 0.5f);
        ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "PAUSED");
        ImGui::SetWindowFontScale(1.0f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button("RESUME", ImVec2(btnW, 40.0f)))
            gamePaused = false;
        PlayHoverSound();

        ImGui::Spacing();

        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button(
                pauseShowSettings ? "SETTINGS  [hide]" : "SETTINGS  [show]",
                ImVec2(btnW, 32.0f))) {
            if (!pauseShowSettings) {
                pendingResIndex   = SaveData::current.resolutionIndex;
                pendingFullscreen = SaveData::current.fullscreen;
                pendingVsync      = SaveData::current.vsyncMode;
                pendingTargetFPS  = SaveData::current.targetFPS;
            }
            pauseShowSettings = !pauseShowSettings;
        }
        PlayHoverSound();

        if (pauseShowSettings) {
            // ── Audio ──────────────────────────────────────────────────────
            ImGui::Spacing();
            ImGui::SetCursorPosX(btnX);
            ImGui::Text("Music Volume");
            ImGui::SetCursorPosX(btnX);
            ImGui::SetNextItemWidth(btnW);
            if (ImGui::SliderFloat("##music", &musicVolume, 0.0f, 1.0f))
                SDL_SetAudioStreamGain(bgmPlayer, musicVolume);

            ImGui::SetCursorPosX(btnX);
            ImGui::Text("SFX Volume");
            ImGui::SetCursorPosX(btnX);
            ImGui::SetNextItemWidth(btnW);
            if (ImGui::SliderFloat("##sfx", &sfxVolume, 0.0f, 1.0f)) {
                SDL_SetAudioStreamGain(sfxPlayer,        sfxVolume);
                SDL_SetAudioStreamGain(sfxLaserHitStream, sfxVolume * 2.0f);
                if (hoverStream) SDL_SetAudioStreamGain(hoverStream, sfxVolume * 0.35f);
            }

            ImGui::SetCursorPosX(btnX);
            if (musicPaused) {
                if (ImGui::Button("Play Music", ImVec2(btnW, 28.0f))) {
                    SDL_ResumeAudioStreamDevice(bgmPlayer);
                    musicPaused = false;
                }
            } else {
                if (ImGui::Button("Pause Music", ImVec2(btnW, 28.0f))) {
                    SDL_PauseAudioStreamDevice(bgmPlayer);
                    musicPaused = true;
                }
            }
            PlayHoverSound();

            // ── Video ──────────────────────────────────────────────────────
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::SetCursorPosX(btnX);
            ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "VIDEO");

            ImGui::SetCursorPosX(btnX);
            ImGui::Text("Resolution");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(btnW - 100.0f);
            ImGui::Combo("##res", &pendingResIndex,
                SaveData::kResolutionLabels, SaveData::kResolutionCount);

            ImGui::SetCursorPosX(btnX);
            ImGui::Checkbox("Fullscreen", &pendingFullscreen);

            ImGui::SetCursorPosX(btnX);
            ImGui::Text("Sync");
            ImGui::SameLine();
            if (ImGui::RadioButton("Adaptive##vs", pendingVsync == -1)) pendingVsync = -1;
            ImGui::SameLine();
            if (ImGui::RadioButton("VSync##vs",    pendingVsync ==  1)) pendingVsync =  1;
            ImGui::SameLine();
            if (ImGui::RadioButton("Off##vs",      pendingVsync ==  0)) pendingVsync =  0;

            // Frame Cap (only meaningful when Sync is Off)
            ImGui::SetCursorPosX(btnX);
            ImGui::Text("Frame Cap");
            ImGui::SameLine();
            {
                static const char* capLabels[] = { "Uncapped", "240 FPS", "144 FPS", "120 FPS", "60 FPS" };
                static const int   capValues[] = { 0, 240, 144, 120, 60 };
                int capIdx = 0;
                for (int ci = 0; ci < 5; ci++) if (capValues[ci] == pendingTargetFPS) { capIdx = ci; break; }
                ImGui::SetNextItemWidth(btnW - 100.0f);
                if (ImGui::Combo("##cap", &capIdx, capLabels, 5))
                    pendingTargetFPS = capValues[capIdx];
            }

            ImGui::Spacing();
            ImGui::SetCursorPosX(btnX);
            if (ImGui::Button("APPLY VIDEO", ImVec2(btnW, 30))) {
                SaveData::current.resolutionIndex = pendingResIndex;
                SaveData::current.fullscreen      = pendingFullscreen;
                SaveData::current.vsyncMode       = pendingVsync;
                SaveData::current.targetFPS       = pendingTargetFPS;
                int w = SaveData::kResolutionW[pendingResIndex];
                int h = SaveData::kResolutionH[pendingResIndex];
                SceneSwitcher::RequestVideo(pendingFullscreen, w, h, pendingVsync);
            }
            PlayHoverSound();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button("BACK TO TITLE", ImVec2(btnW, 36.0f))) {
            gamePaused = false;
            SceneSwitcher::Request(GameScene::TITLE);
        }
        PlayHoverSound();

        ImGui::Spacing();

        ImGui::SetCursorPosX(btnX);
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.50f, 0.10f, 0.10f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.00f, 0.20f, 0.20f, 1.0f));
        if (ImGui::Button("QUIT GAME", ImVec2(btnW, 36.0f))) {
            SDL_Event e; e.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&e);
        }
        PlayHoverSound();
        ImGui::PopStyleColor(3);

    ImGui::End();
}

// ── Game Over screen ──────────────────────────────────────────────────────────
void SceneMuntasir::DrawGameOver() {
    if (!gameOver) return;

    ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(
            ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 220), ImGuiCond_Always);
        ImGui::Begin("##gameover", nullptr,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove   |
            ImGuiWindowFlags_NoTitleBar);

        ImGui::SetWindowFontScale(2.0f);
        ImVec2 goSz = ImGui::CalcTextSize("GAME OVER");
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - goSz.x) * 0.5f);
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "GAME OVER");
        ImGui::SetWindowFontScale(1.0f);

        ImGui::Spacing();
        ImGui::SetCursorPosX(100);
        ImGui::Text("Final Score: %d", score);
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::SetCursorPosX(50);
        if (ImGui::Button("Try Again", ImVec2(120, 40))) {
            gameOver   = false;
            score      = 0;
            shardCount = 0;
            shards.clear();
            autoSaveTimer = 0.0f;
            currentPhase  = 1;
            player->Reset();
            asteroid->Reset();
            bot01->Reset();
            bot02->Reset();
            levelDirector->Reset();
            prevLives = player->GetLives();
            SaveData::current.lives    = 3;
            SaveData::current.health   = 100.0f;
            SaveData::current.score    = 0;
            SaveData::current.posX     = 0.0f;
            SaveData::current.posY     = 0.0f;
            SaveData::current.waveTime = 0.0f;
            SaveData::current.shardCount = 0;
            SaveData::current.Save();
        }
        PlayHoverSound();
        ImGui::SameLine();
        if (ImGui::Button("Title", ImVec2(70, 40)))
            SceneSwitcher::Request(GameScene::TITLE);
        PlayHoverSound();
        ImGui::SameLine();
        if (ImGui::Button("Exit", ImVec2(70, 40))) {
            SDL_Event e; e.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&e);
        }
        PlayHoverSound();
        ImGui::End();
}