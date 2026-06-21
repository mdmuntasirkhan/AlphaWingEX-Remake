#include <glew.h>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <SDL.h>
#include <SDL3/SDL_events.h>
#include "SceneMuntasir.h"
#include <MMath.h>
#include "Debug.h"
#include "Mesh.h"
#include "Shader.h"
#include "imgui.h"

// Constructor
SceneMuntasir::SceneMuntasir() :
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
    lostShards{},
    hasLostShards{ false },
    prevLives{ 3 },
    autoSaveTimer{ 0.0f },
    explosionCooldown{ 2.0f },
    explosionCooldownTimer{ 0.0f },
    audioPlayer{ nullptr },
    sfxPlayer{ nullptr },
    sfxLaser{ nullptr },
    sfxExplosion{ nullptr },
    sfxMissileHit{ nullptr },
    audioTest{ nullptr },
    musicVolume{ 0.1f },
    sfxVolume{ 0.05f },
    musicPaused{ false },
    gamePaused{ false },
    pauseShowSettings{ false },
    pendingResIndex{ SaveData::current.resolutionIndex },
    pendingFullscreen{ SaveData::current.fullscreen },
    pendingVsync{ SaveData::current.vsyncMode },
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

    // Environment - starfield
    environment = new Environment();
    if (environment->OnCreate(1920.0f, 1080.0f) == false) {
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
    projectionMatrix = MMath::perspective(45.0f, 16.0f / 9.0f, 0.1f, 100.0f);

    // Audio Setup
    SDL_AudioSpec defaultSpec;
    defaultSpec.freq = 44100;
    defaultSpec.channels = 2;
    defaultSpec.format = SDL_AUDIO_S16;

    // Music stream
    audioPlayer = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &defaultSpec, nullptr, nullptr);
    if (!audioPlayer) std::cout << "Failed to create music player\n";
    SDL_SetAudioStreamGain(audioPlayer, musicVolume);
    SDL_ResumeAudioStreamDevice(audioPlayer);

    // SFX stream
    sfxPlayer = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &defaultSpec, nullptr, nullptr);
    if (!sfxPlayer) std::cout << "Failed to create SFX player\n";
    SDL_SetAudioStreamGain(sfxPlayer, sfxVolume);
    SDL_ResumeAudioStreamDevice(sfxPlayer);

    // Music
    audioTest = new Sound("audio/music/deadmou5-gg.wav");
    audioTest->OnCreate();
    audioTest->Play(audioPlayer);

    // SFX
    sfxLaser = new Sound("audio/sfx/LaserShoot.wav");
    sfxLaser->OnCreate();

    sfxExplosion = new Sound("audio/sfx/Explosion03.wav");
    sfxExplosion->OnCreate();

    sfxMissileHit = new Sound("audio/sfx/missileHit.wav");
    sfxMissileHit->OnCreate();

    hoverStream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &defaultSpec, nullptr, nullptr);
    if (hoverStream) {
        SDL_SetAudioStreamGain(hoverStream, sfxVolume * 0.35f);
        SDL_ResumeAudioStreamDevice(hoverStream);
    }
    uiClickSound = new Sound("audio/sfx/Select01.wav");
    uiClickSound->OnCreate();

    SDL_Log("Music queued bytes: %d", SDL_GetAudioStreamQueued(audioPlayer));
    SDL_Log("SFX queued bytes: %d", SDL_GetAudioStreamQueued(sfxPlayer));

    // Restore full game state from save (works for both New Game and Load Game)
    shardCount = SaveData::current.shardCount;
    score      = SaveData::current.score;

    player->SetHealth  (SaveData::current.health);
    player->SetLives   (SaveData::current.lives);
    player->SetPosition(SaveData::current.posX, SaveData::current.posY);

    bot01->SetTotalTime(SaveData::current.waveTime);

    // Restore lost shard pile
    hasLostShards = SaveData::current.hasLostShards;
    if (hasLostShards) {
        lostShards.pos   = Vec3(SaveData::current.lostShardPosX,
                                SaveData::current.lostShardPosY, -10.0f);
        lostShards.count = SaveData::current.lostShardCount;
        lostShards.pulseTimer = 0.0f;
    }

    prevLives = player->GetLives();

    // Apply saved audio preferences
    SDL_SetAudioStreamGain(audioPlayer, SaveData::current.musicVolume);
    SDL_SetAudioStreamGain(sfxPlayer,   SaveData::current.sfxVolume);
    musicVolume = SaveData::current.musicVolume;
    sfxVolume   = SaveData::current.sfxVolume;

    return true;
}

// OnDestroy
void SceneMuntasir::OnDestroy() {
    Debug::Info("Deleting assets SceneMuntasir: ", __FILE__, __LINE__);

    // Auto-save full state on scene exit (covers quit, title-return, etc.)
    SaveGame();

    // Shards
    shards.clear();
    if (shardMesh) {
        shardMesh->OnDestroy();
        delete shardMesh;
        shardMesh = nullptr;
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

    sfxExplosion->OnDestroy();
    delete sfxExplosion;

    sfxMissileHit->OnDestroy();
    delete sfxMissileHit;

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
    SDL_DestroyAudioStream(audioPlayer);
    SDL_DestroyAudioStream(sfxPlayer);
    audioTest->OnDestroy();
    delete audioTest;
}

// HandleEvents
void SceneMuntasir::HandleEvents(const SDL_Event& sdlEvent) {
    switch (sdlEvent.type) {
    case SDL_EVENT_KEY_DOWN:
        switch (sdlEvent.key.scancode) {
        case SDL_SCANCODE_ESCAPE:
            if (!gameOver) gamePaused = !gamePaused;
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
            if (!ImGui::GetIO().WantCaptureMouse) {
                bool launched = false;
                if (bot01->GetBot01Positions().size() > 0) {
                    bullet->SpawnHoming(player->GetPosition(),
                        MissileTargetType::BOT01, 0);
                    launched = true;
                }
                else if (asteroid->GetAsteroidPositions().size() > 0) {
                    bullet->SpawnHoming(player->GetPosition(),
                        MissileTargetType::ASTEROID, 0);
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
    SaveData::current.hasLostShards   = hasLostShards;
    SaveData::current.lostShardPosX   = hasLostShards ? lostShards.pos.x : 0.0f;
    SaveData::current.lostShardPosY   = hasLostShards ? lostShards.pos.y : 0.0f;
    SaveData::current.lostShardCount  = hasLostShards ? lostShards.count : 0;
    SaveData::current.musicVolume     = musicVolume;
    SaveData::current.sfxVolume       = sfxVolume;
    if (score > SaveData::current.highScore)
        SaveData::current.highScore   = score;
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
    if (gamePaused) return;

    static float totalTime = 0.0f;
    totalTime += deltaTime;

    // Cooldown timers
    if (explosionCooldownTimer > 0.0f)
        explosionCooldownTimer -= deltaTime;

    // Periodic auto-save (only while alive)
    if (!gameOver) {
        autoSaveTimer += deltaTime;
        if (autoSaveTimer >= kAutoSaveInterval) {
            autoSaveTimer = 0.0f;
            SaveGame();
        }
    }

    if (gameOver) return;

    // Update all classes
    player->Update(deltaTime);
    bullet->Update(deltaTime,
        asteroid->GetAsteroidPositions(), Vec3(-asteroid->GetAsteroidSpeed(), 0.0f, 0.0f),
        bot01->GetBot01Positions(), Vec3(-bot01->GetBot01Speed(), 0.0f, 0.0f));
    asteroid->Update(deltaTime);
    bot01->Update(deltaTime, 0.0f, player->GetPosition().y);
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
        if (shards[i].pos.x < -16.0f || shards[i].pos.x > 16.0f ||
            shards[i].pos.y < -10.0f || shards[i].pos.y >  10.0f) {
            shards.erase(shards.begin() + i);
        }
    }

    // Lost shard pile — pulse + collect (larger 1.2-unit radius so it feels good)
    if (hasLostShards) {
        lostShards.pulseTimer += deltaTime;
        float ldx = ppos.x - lostShards.pos.x;
        float ldy = ppos.y - lostShards.pos.y;
        if (ldx*ldx + ldy*ldy < 1.2f * 1.2f) {
            shardCount   += lostShards.count;
            hasLostShards = false;
            SaveGame();   // immediately bank the recovered shards
        }
    }

    // Check game over
    if (player->IsGameOver()) {
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
    for (int m = (int)bullet->GetMissilePositions().size() - 1; m >= 0; m--) {
        for (int e = (int)bot01->GetBot01Positions().size() - 1; e >= 0; e--) {
            float dx = bullet->GetMissilePositions()[m].x - bot01->GetBot01Positions()[e].x;
            float dy = bullet->GetMissilePositions()[m].y - bot01->GetBot01Positions()[e].y;
            if ((dx*dx)/(0.65f*0.65f) + (dy*dy)/(0.35f*0.35f) < 1.0f) {
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
                // Stagger Bot02 position — hover system pulls it back, creating
                // a visible recoil-and-recover motion proportional to impact angle
                bot02->PushPosition(e, iDirX2 * 0.55f, iDirY2 * 0.35f);
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

    // Wave trigger — all 5 Bot01s killed → spawn Bot02 pair on right side
    if (bot01->IsWaveComplete() && bot02->GetCount() == 0) {
        bot02->Spawn(player->GetPosition().y);
        bot01->ResetWave();
    }

    // Life-loss detection — after all collision damage this frame.
    int currentLives = player->GetLives();
    if (currentLives < prevLives) {
        if (shardCount > 0) {
            // Drop shards at death position — replaces any previous pile
            lostShards.pos        = player->GetPosition();
            lostShards.count      = shardCount;
            lostShards.pulseTimer = 0.0f;
            hasLostShards         = true;
            shardCount            = 0;
        }
        // If shardCount == 0: keep any existing pile — dying broke doesn't erase
        // previously dropped shards (player still has a chance to recover them)
        SaveGame();  // snapshot: one fewer life, 0 shards, pile recorded
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

    asteroid->Render(shader, projectionMatrix, viewMatrix);
    bot01->Render(shader, projectionMatrix, viewMatrix);
    bot02->Render(shader, projectionMatrix, viewMatrix);

    // Energy shards + lost shard pile — additive emissive
    if (!shards.empty() || hasLostShards) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        glUniform1f(shader->GetUniformID("emissive"), 1.0f);

        // Regular shards — spinning gold orbs (scale 0.12, visible now)
        glUniform4f(shader->GetUniformID("color"), 1.0f, 0.85f, 0.1f, 0.9f);
        for (int i = 0; i < (int)shards.size(); i++) {
            Matrix4 m = MMath::translate(shards[i].pos) *
                        MMath::rotate(shards[i].angle, Vec3(0.0f, 0.0f, 1.0f)) *
                        MMath::scale(0.12f, 0.12f, 0.12f);
            glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
            shardMesh->Render();
        }

        // Lost shard pile — larger pulsing white-gold cluster at death position
        if (hasLostShards) {
            float pulse = 0.55f + 0.45f * sinf(lostShards.pulseTimer * 4.5f);
            glUniform4f(shader->GetUniformID("color"), 1.0f, 0.92f, 0.4f, pulse);
            // Draw 3 orbs in a small cluster around the drop point
            const Vec3 offsets[3] = {
                Vec3( 0.00f,  0.00f, 0.0f),
                Vec3( 0.18f,  0.10f, 0.0f),
                Vec3(-0.18f,  0.10f, 0.0f)
            };
            for (int k = 0; k < 3; k++) {
                Vec3 p = lostShards.pos + offsets[k];
                float spin = lostShards.pulseTimer * 60.0f + k * 120.0f;
                Matrix4 m = MMath::translate(p) *
                            MMath::rotate(spin, Vec3(0.0f, 0.0f, 1.0f)) *
                            MMath::scale(0.18f, 0.18f, 0.18f);
                glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
                shardMesh->Render();
            }
        }

        glUniform1f(shader->GetUniformID("emissive"), 0.0f);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    glUseProgram(0);

    // Stars — ImGui draws these on top of 3D (fine; they're small background dots)
    environment->Render();
}

// DrawGui
void SceneMuntasir::DrawGui() {

    auto chkHov = [&]() {
        if (!uiClickSound || !hoverStream || !ImGui::IsItemHovered()) return;
        unsigned int id  = ImGui::GetItemID();
        Uint64       now = SDL_GetTicks();
        if (id == lastHoveredId || now - lastHoverTick < 150) return;
        lastHoveredId = id;
        lastHoverTick = now;
        uiClickSound->Play(hoverStream);
    };

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
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 1.0f),
        "MISSILES: %d / %d", bullet->GetMissileCount(), bullet->GetMaxMissiles());

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

    // Shield status
    if (player->IsShieldActive()) {
        ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "SHIELD ACTIVE!");
    } else if (player->IsShieldOnCooldown()) {
        ImGui::Text("Shield Recharging...");
        ImGui::ProgressBar(player->GetShieldCooldownPercent(), ImVec2(250, 12), "");
    } else {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "SHIELD READY  [E]");
    }

    // Lost shard warning
    if (hasLostShards) {
        ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.05f, 1.0f),
            ">> LOST SHARDS: %d", lostShards.count);
    }

    ImGui::TextDisabled("ESC  Pause");

    ImGui::End();

    // ── Pause Menu ────────────────────────────────────────────────────────
    if (gamePaused) {
        ImGuiIO& io   = ImGui::GetIO();
        float    panW = 380.0f;
        float    panH = pauseShowSettings ? 590.0f : 275.0f;
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
        chkHov();

        ImGui::Spacing();

        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button(
                pauseShowSettings ? "SETTINGS  [hide]" : "SETTINGS  [show]",
                ImVec2(btnW, 32.0f))) {
            if (!pauseShowSettings) {
                pendingResIndex   = SaveData::current.resolutionIndex;
                pendingFullscreen = SaveData::current.fullscreen;
                pendingVsync      = SaveData::current.vsyncMode;
            }
            pauseShowSettings = !pauseShowSettings;
        }
        chkHov();

        if (pauseShowSettings) {
            // ── Audio ──────────────────────────────────────────────────────
            ImGui::Spacing();
            ImGui::SetCursorPosX(btnX);
            ImGui::Text("Music Volume");
            ImGui::SetCursorPosX(btnX);
            ImGui::SetNextItemWidth(btnW);
            if (ImGui::SliderFloat("##music", &musicVolume, 0.0f, 1.0f))
                SDL_SetAudioStreamGain(audioPlayer, musicVolume);

            ImGui::SetCursorPosX(btnX);
            ImGui::Text("SFX Volume");
            ImGui::SetCursorPosX(btnX);
            ImGui::SetNextItemWidth(btnW);
            if (ImGui::SliderFloat("##sfx", &sfxVolume, 0.0f, 1.0f)) {
                SDL_SetAudioStreamGain(sfxPlayer,   sfxVolume);
                if (hoverStream) SDL_SetAudioStreamGain(hoverStream, sfxVolume * 0.35f);
            }

            ImGui::SetCursorPosX(btnX);
            if (musicPaused) {
                if (ImGui::Button("Play Music", ImVec2(btnW, 28.0f))) {
                    SDL_ResumeAudioStreamDevice(audioPlayer);
                    musicPaused = false;
                }
            } else {
                if (ImGui::Button("Pause Music", ImVec2(btnW, 28.0f))) {
                    SDL_PauseAudioStreamDevice(audioPlayer);
                    musicPaused = true;
                }
            }
            chkHov();

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

            ImGui::Spacing();
            ImGui::SetCursorPosX(btnX);
            if (ImGui::Button("APPLY VIDEO", ImVec2(btnW, 30))) {
                SaveData::current.resolutionIndex = pendingResIndex;
                SaveData::current.fullscreen      = pendingFullscreen;
                SaveData::current.vsyncMode       = pendingVsync;
                int w = SaveData::kResolutionW[pendingResIndex];
                int h = SaveData::kResolutionH[pendingResIndex];
                SceneSwitcher::RequestVideo(pendingFullscreen, w, h, pendingVsync);
            }
            chkHov();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button("BACK TO TITLE", ImVec2(btnW, 36.0f))) {
            gamePaused = false;
            SceneSwitcher::Request(GameScene::TITLE);
        }
        chkHov();

        ImGui::Spacing();

        ImGui::SetCursorPosX(btnX);
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.50f, 0.10f, 0.10f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.00f, 0.20f, 0.20f, 1.0f));
        if (ImGui::Button("QUIT GAME", ImVec2(btnW, 36.0f))) {
            SDL_Event e; e.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&e);
        }
        chkHov();
        ImGui::PopStyleColor(3);

        ImGui::End();
    }

    // ── Game Over screen ──────────────────────────────────────────────────
    if (gameOver) {
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
            gameOver      = false;
            score         = 0;
            shardCount    = 0;
            hasLostShards = false;
            shards.clear();
            autoSaveTimer = 0.0f;
            player->Reset();
            asteroid->Reset();
            bot01->Reset();
            bot02->Reset();
            prevLives = player->GetLives();
            SaveData::current.Reset();
            SaveData::current.Save();
        }
        chkHov();
        ImGui::SameLine();
        if (ImGui::Button("Title", ImVec2(70, 40)))
            SceneSwitcher::Request(GameScene::TITLE);
        chkHov();
        ImGui::SameLine();
        if (ImGui::Button("Exit", ImVec2(70, 40))) {
            SDL_Event e; e.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&e);
        }
        chkHov();
        ImGui::End();
    }
}