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
    enemy{ nullptr },
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
    audioTest{ nullptr },
    musicVolume{ 0.1f },
    sfxVolume{ 0.05f },
    musicPaused{ false } {
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

    // Enemy - handles asteroids and Bot01
    enemy = new Enemy();
    if (enemy->OnCreate("meshes/Temp_Asteroid.obj",
        "meshes/Temp_AlphaWing_Enemy_Bot01.obj") == false) {
        std::cout << "Enemy failed to load!\n";
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

    SDL_Log("Music queued bytes: %d", SDL_GetAudioStreamQueued(audioPlayer));
    SDL_Log("SFX queued bytes: %d", SDL_GetAudioStreamQueued(sfxPlayer));

    // Restore full game state from save (works for both New Game and Load Game)
    shardCount = SaveData::current.shardCount;
    score      = SaveData::current.score;

    player->SetHealth  (SaveData::current.health);
    player->SetLives   (SaveData::current.lives);
    player->SetPosition(SaveData::current.posX, SaveData::current.posY);

    enemy->SetTotalTime(SaveData::current.waveTime);

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

    enemy->OnDestroy();
    delete enemy;
    enemy = nullptr;

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
                if (enemy->GetBot01Positions().size() > 0) {
                    bullet->SpawnHoming(player->GetPosition(),
                        MissileTargetType::BOT01, 0);
                }
                else if (enemy->GetAsteroidPositions().size() > 0) {
                    bullet->SpawnHoming(player->GetPosition(),
                        MissileTargetType::ASTEROID, 0);
                }
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
    SaveData::current.waveTime        = enemy->GetTotalTime();
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
    static float totalTime = 0.0f;
    totalTime += deltaTime;

    // Explosion cooldown timer
    if (explosionCooldownTimer > 0.0f) {
        explosionCooldownTimer -= deltaTime;
    }

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
        enemy->GetAsteroidPositions(), Vec3(-enemy->GetAsteroidSpeed(), 0.0f, 0.0f),
        enemy->GetBot01Positions(), Vec3(-enemy->GetBot01Speed(), 0.0f, 0.0f));
    enemy->Update(deltaTime, player->GetPosition().y);
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
            // Small auto-save nudge so shard count is banked continuously
            SaveData::current.shardCount = shardCount;
            SaveData::current.Save();
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
        for (int e = enemy->GetBot01Positions().size() - 1; e >= 0; e--) {
            float dx = bullet->GetPositions()[b].x - enemy->GetBot01Positions()[e].x;
            float dy = bullet->GetPositions()[b].y - enemy->GetBot01Positions()[e].y;
            if ((dx*dx)/(0.6f*0.6f) + (dy*dy)/(0.32f*0.32f) < 1.0f) {
                Vec3 deathPos = enemy->GetBot01Positions()[e];
                bullet->RemoveAt(b);
                if (enemy->DamageBot01(e)) {
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
        for (int a = enemy->GetAsteroidPositions().size() - 1; a >= 0; a--) {
            float dx = bullet->GetPositions()[b].x - enemy->GetAsteroidPositions()[a].x;
            float dy = bullet->GetPositions()[b].y - enemy->GetAsteroidPositions()[a].y;
            if ((dx*dx)/(0.85f*0.85f) + (dy*dy)/(0.7f*0.7f) < 1.0f) {
                Vec3 deathPos = enemy->GetAsteroidPositions()[a];
                bullet->RemoveAt(b);
                if (enemy->DamageAsteroid(a)) {
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
        for (int a = enemy->GetSmallAsteroidPositions().size() - 1; a >= 0; a--) {
            float dx = bullet->GetPositions()[b].x - enemy->GetSmallAsteroidPositions()[a].x;
            float dy = bullet->GetPositions()[b].y - enemy->GetSmallAsteroidPositions()[a].y;
            if ((dx*dx)/(0.45f*0.45f) + (dy*dy)/(0.38f*0.38f) < 1.0f) {
                Vec3 deathPos = enemy->GetSmallAsteroidPositions()[a];
                bullet->RemoveAt(b);
                if (enemy->DamageSmallAsteroid(a)) {
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

    // Missile hits Bot01
    for (int m = bullet->GetMissilePositions().size() - 1; m >= 0; m--) {
        for (int e = enemy->GetBot01Positions().size() - 1; e >= 0; e--) {
            float dx = bullet->GetMissilePositions()[m].x - enemy->GetBot01Positions()[e].x;
            float dy = bullet->GetMissilePositions()[m].y - enemy->GetBot01Positions()[e].y;
            if ((dx*dx)/(0.65f*0.65f) + (dy*dy)/(0.35f*0.35f) < 1.0f) {
                Vec3 deathPos = enemy->GetBot01Positions()[e];
                bullet->RemoveMissileAt(m);
                if (enemy->DamageBot01(e)) {
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

    // Missile hits large asteroid
    for (int m = bullet->GetMissilePositions().size() - 1; m >= 0; m--) {
        for (int a = enemy->GetAsteroidPositions().size() - 1; a >= 0; a--) {
            float dx = bullet->GetMissilePositions()[m].x - enemy->GetAsteroidPositions()[a].x;
            float dy = bullet->GetMissilePositions()[m].y - enemy->GetAsteroidPositions()[a].y;
            if ((dx*dx)/(0.9f*0.9f) + (dy*dy)/(0.75f*0.75f) < 1.0f) {
                Vec3 deathPos = enemy->GetAsteroidPositions()[a];
                bullet->RemoveMissileAt(m);
                if (enemy->DamageAsteroid(a)) {
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

    // Missile hits small asteroid
    for (int m = bullet->GetMissilePositions().size() - 1; m >= 0; m--) {
        for (int a = enemy->GetSmallAsteroidPositions().size() - 1; a >= 0; a--) {
            float dx = bullet->GetMissilePositions()[m].x - enemy->GetSmallAsteroidPositions()[a].x;
            float dy = bullet->GetMissilePositions()[m].y - enemy->GetSmallAsteroidPositions()[a].y;
            if ((dx*dx)/(0.5f*0.5f) + (dy*dy)/(0.4f*0.4f) < 1.0f) {
                Vec3 deathPos = enemy->GetSmallAsteroidPositions()[a];
                bullet->RemoveMissileAt(m);
                if (enemy->DamageSmallAsteroid(a)) {
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

    // Asteroid hits player
    for (int a = enemy->GetAsteroidPositions().size() - 1; a >= 0; a--) {
        float dx = player->GetPosition().x - enemy->GetAsteroidPositions()[a].x;
        float dy = player->GetPosition().y - enemy->GetAsteroidPositions()[a].y;
        if ((dx*dx)/(1.0f*1.0f) + (dy*dy)/(0.5f*0.5f) < 1.0f) {
            Vec3 deathPos = enemy->GetAsteroidPositions()[a];
            float len = sqrtf(dx*dx + dy*dy);
            if (len > 0.001f) player->ApplyImpulse(Vec3(dx/len * 4.0f, dy/len * 4.0f, 0.0f));
            enemy->RemoveAsteroid(a);
            SpawnShards(deathPos, 3);
            player->TakeDamage(25.0f);
            if (explosionCooldownTimer <= 0.0f) {
                sfxExplosion->Play(sfxPlayer);
                explosionCooldownTimer = explosionCooldown;
            }
        }
    }

    // Small asteroid hits player
    for (int a = enemy->GetSmallAsteroidPositions().size() - 1; a >= 0; a--) {
        float dx = player->GetPosition().x - enemy->GetSmallAsteroidPositions()[a].x;
        float dy = player->GetPosition().y - enemy->GetSmallAsteroidPositions()[a].y;
        if ((dx*dx)/(0.65f*0.65f) + (dy*dy)/(0.35f*0.35f) < 1.0f) {
            Vec3 deathPos = enemy->GetSmallAsteroidPositions()[a];
            float len = sqrtf(dx*dx + dy*dy);
            if (len > 0.001f) player->ApplyImpulse(Vec3(dx/len * 2.5f, dy/len * 2.5f, 0.0f));
            enemy->RemoveSmallAsteroid(a);
            SpawnShards(deathPos, 2);
            player->TakeDamage(10.0f);
            if (explosionCooldownTimer <= 0.0f) {
                sfxExplosion->Play(sfxPlayer);
                explosionCooldownTimer = explosionCooldown;
            }
        }
    }

    // Bot01 hits player
    for (int e = enemy->GetBot01Positions().size() - 1; e >= 0; e--) {
        float dx = player->GetPosition().x - enemy->GetBot01Positions()[e].x;
        float dy = player->GetPosition().y - enemy->GetBot01Positions()[e].y;
        if ((dx*dx)/(0.75f*0.75f) + (dy*dy)/(0.4f*0.4f) < 1.0f) {
            Vec3 deathPos = enemy->GetBot01Positions()[e];
            float len = sqrtf(dx*dx + dy*dy);
            if (len > 0.001f) player->ApplyImpulse(Vec3(dx/len * 5.0f, dy/len * 5.0f, 0.0f));
            enemy->RemoveBot01(e);
            SpawnShards(deathPos, 5);
            player->TakeDamage(40.0f);
            if (explosionCooldownTimer <= 0.0f) {
                sfxExplosion->Play(sfxPlayer);
                explosionCooldownTimer = explosionCooldown;
            }
        }
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
    glUniform3f(shader->GetUniformID("lightPos"), 0.0f, 5.0f, 10.0f);
    glUniform3f(shader->GetUniformID("viewPos"), 0.0f, 0.0f, 10.0f);

    // Player — colors set per-part inside Player::Render()
    player->Render(shader, projectionMatrix, viewMatrix);

    // Bullets — YELLOW
    glUniform4f(shader->GetUniformID("color"), 1.0f, 1.0f, 0.0f, 1.0f);
    bullet->Render(shader, projectionMatrix, viewMatrix);

    enemy->Render(shader, projectionMatrix, viewMatrix);

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

    // HUD
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300, 290), ImGuiCond_Always);
    ImGui::Begin("Alpha Wing EX", nullptr, ImGuiWindowFlags_NoResize);

    // Pilot name + score
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f),
        "PILOT: %s", SaveData::current.profileName.c_str());
    ImGui::Text("SCORE: %d", score);

    // Shards
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.1f, 1.0f), "SHARDS: %d", shardCount);

    // Lives
    ImGui::Text("LIVES: ");
    ImGui::SameLine();
    for (int i = 0; i < player->GetLives(); i++) {
        ImGui::Text("[*]");
        ImGui::SameLine();
    }
    ImGui::NewLine();

    // Health bar
    float healthPercent = player->GetHealth() / 100.0f;
    if (healthPercent > 0.6f) {
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
            ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    }
    else if (healthPercent > 0.3f) {
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
            ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
    }
    else {
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
            ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
    }
    ImGui::ProgressBar(healthPercent, ImVec2(260, 20), "");
    ImGui::PopStyleColor();

    // Shield
    if (player->IsShieldActive()) {
        ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "SHIELD ACTIVE!");
    }
    else if (player->IsShieldOnCooldown()) {
        ImGui::Text("Shield Recharging...");
        ImGui::ProgressBar(player->GetShieldCooldownPercent(),
            ImVec2(260, 15), "");
    }
    else {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f),
            "SHIELD READY (E)");
    }

    // Lost shard warning
    if (hasLostShards) {
        ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.05f, 1.0f),
            ">> LOST SHARDS: %d  (recover or lose!)", lostShards.count);
    }

    ImGui::Separator();

    // Save & return to title
    if (ImGui::Button("Save & Title", ImVec2(127, 28))) {
        SceneSwitcher::Request(GameScene::TITLE); // OnDestroy auto-saves
    }
    ImGui::SameLine();
    if (ImGui::Button("Exit Game", ImVec2(127, 28))) {
        SDL_Event quitEvent;
        quitEvent.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quitEvent);
    }

    ImGui::Separator();

    // Music volume
    ImGui::Text("Music Volume");
    if (ImGui::SliderFloat("##music", &musicVolume, 0.0f, 1.0f)) {
        SDL_SetAudioStreamGain(audioPlayer, musicVolume);
    }

    // SFX volume
    ImGui::Text("SFX Volume");
    if (ImGui::SliderFloat("##sfx", &sfxVolume, 0.0f, 1.0f)) {
        SDL_SetAudioStreamGain(sfxPlayer, sfxVolume);
    }

    // Pause/Play
    if (musicPaused) {
        if (ImGui::Button("Play Music", ImVec2(260, 28))) {
            SDL_ResumeAudioStreamDevice(audioPlayer);
            musicPaused = false;
        }
    }
    else {
        if (ImGui::Button("Pause Music", ImVec2(260, 28))) {
            SDL_PauseAudioStreamDevice(audioPlayer);
            musicPaused = true;
        }
    }

    ImGui::End();

    // Game Over screen
    if (gameOver) {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(
            ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 220), ImGuiCond_Always);
        ImGui::Begin("##gameover", nullptr,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoTitleBar);

        ImGui::SetCursorPosX(130);
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "GAME OVER");
        ImGui::Spacing();
        ImGui::SetCursorPosX(100);
        ImGui::Text("Final Score: %d", score);
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::SetCursorPosX(50);
        if (ImGui::Button("Try Again", ImVec2(120, 40))) {
            // Full game over: restart run but keep profile/highscore on disk
            gameOver      = false;
            score         = 0;
            shardCount    = 0;
            hasLostShards = false;
            shards.clear();
            autoSaveTimer = 0.0f;
            player->Reset();
            enemy->Reset();
            prevLives = player->GetLives();
            SaveData::current.Reset();
            SaveData::current.Save();  // wipe the in-progress save cleanly
        }
        ImGui::SameLine();
        if (ImGui::Button("Title", ImVec2(70, 40))) {
            SceneSwitcher::Request(GameScene::TITLE); // OnDestroy auto-saves high score
        }
        ImGui::SameLine();
        if (ImGui::Button("Exit", ImVec2(70, 40))) {
            SDL_Event quitEvent;
            quitEvent.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&quitEvent);
        }
        ImGui::End();
    }
}