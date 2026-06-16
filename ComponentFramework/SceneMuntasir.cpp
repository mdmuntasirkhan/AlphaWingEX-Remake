#include <glew.h>
#include <iostream>
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
    explosionCooldown{ 2.0f },
    explosionCooldownTimer{ 0.0f },
    audioPlayer{ nullptr },
    sfxPlayer{ nullptr },
    sfxLaser{ nullptr },
    sfxExplosion{ nullptr },
    audioTest{ nullptr },
    musicVolume{ 0.3f },
    sfxVolume{ 0.3f },
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
    SDL_ResumeAudioStreamDevice(audioPlayer);

    // SFX stream
    sfxPlayer = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &defaultSpec, nullptr, nullptr);
    if (!sfxPlayer) std::cout << "Failed to create SFX player\n";
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

    return true;
}

// OnDestroy
void SceneMuntasir::OnDestroy() {
    Debug::Info("Deleting assets SceneMuntasir: ", __FILE__, __LINE__);

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

// Update
void SceneMuntasir::Update(const float deltaTime) {
    static float totalTime = 0.0f;
    totalTime += deltaTime;

    // Explosion cooldown timer
    if (explosionCooldownTimer > 0.0f) {
        explosionCooldownTimer -= deltaTime;
    }

    if (gameOver) return;

    // Update all classes
    player->Update(deltaTime);
    enemy->Update(deltaTime);
    bullet->Update(deltaTime, enemy->GetAsteroidPositions(), enemy->GetBot01Positions());
    environment->Update(deltaTime);

    // Check game over
    if (player->IsGameOver()) {
        gameOver = true;
    }

    // Collision - bullet hits Bot01
    for (int b = bullet->GetPositions().size() - 1; b >= 0; b--) {
        for (int e = enemy->GetBot01Positions().size() - 1; e >= 0; e--) {
            float dx = bullet->GetPositions()[b].x - enemy->GetBot01Positions()[e].x;
            float dy = bullet->GetPositions()[b].y - enemy->GetBot01Positions()[e].y;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < 1.0f) {
                bullet->RemoveAt(b);
                enemy->RemoveBot01(e);
                score += 100;
                if (explosionCooldownTimer <= 0.0f) {
                    sfxExplosion->Play(sfxPlayer);
                    explosionCooldownTimer = explosionCooldown;
                }
                break;
            }
        }
    }

    // Collision - bullet hits asteroid
    for (int b = bullet->GetPositions().size() - 1; b >= 0; b--) {
        for (int a = enemy->GetAsteroidPositions().size() - 1; a >= 0; a--) {
            float dx = bullet->GetPositions()[b].x - enemy->GetAsteroidPositions()[a].x;
            float dy = bullet->GetPositions()[b].y - enemy->GetAsteroidPositions()[a].y;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < 1.0f) {
                bullet->RemoveAt(b);
                enemy->RemoveAsteroid(a);
                score += 50;
                if (explosionCooldownTimer <= 0.0f) {
                    sfxExplosion->Play(sfxPlayer);
                    explosionCooldownTimer = explosionCooldown;
                }
                break;
            }
        }
    }

    // Collision - missile hits Bot01
    for (int m = bullet->GetMissilePositions().size() - 1; m >= 0; m--) {
        for (int e = enemy->GetBot01Positions().size() - 1; e >= 0; e--) {
            float dx = bullet->GetMissilePositions()[m].x - enemy->GetBot01Positions()[e].x;
            float dy = bullet->GetMissilePositions()[m].y - enemy->GetBot01Positions()[e].y;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < 1.0f) {
                bullet->RemoveMissileAt(m);
                enemy->RemoveBot01(e);
                score += 100;
                if (explosionCooldownTimer <= 0.0f) {
                    sfxExplosion->Play(sfxPlayer);
                    explosionCooldownTimer = explosionCooldown;
                }
                break;
            }
        }
    }

    // Collision - missile hits asteroid
    for (int m = bullet->GetMissilePositions().size() - 1; m >= 0; m--) {
        for (int a = enemy->GetAsteroidPositions().size() - 1; a >= 0; a--) {
            float dx = bullet->GetMissilePositions()[m].x - enemy->GetAsteroidPositions()[a].x;
            float dy = bullet->GetMissilePositions()[m].y - enemy->GetAsteroidPositions()[a].y;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < 1.0f) {
                bullet->RemoveMissileAt(m);
                enemy->RemoveAsteroid(a);
                score += 50;
                if (explosionCooldownTimer <= 0.0f) {
                    sfxExplosion->Play(sfxPlayer);
                    explosionCooldownTimer = explosionCooldown;
                }
                break;
            }
        }
    }

    // Collision - asteroid hits player
    for (int a = enemy->GetAsteroidPositions().size() - 1; a >= 0; a--) {
        float dx = player->GetPosition().x - enemy->GetAsteroidPositions()[a].x;
        float dy = player->GetPosition().y - enemy->GetAsteroidPositions()[a].y;
        float distance = sqrt(dx * dx + dy * dy);
        if (distance < 1.0f) {
            enemy->RemoveAsteroid(a);
            player->TakeDamage(25.0f);
            if (explosionCooldownTimer <= 0.0f) {
                sfxExplosion->Play(sfxPlayer);
                explosionCooldownTimer = explosionCooldown;
            }
        }
    }

    // Collision - Bot01 hits player
    for (int e = enemy->GetBot01Positions().size() - 1; e >= 0; e--) {
        float dx = player->GetPosition().x - enemy->GetBot01Positions()[e].x;
        float dy = player->GetPosition().y - enemy->GetBot01Positions()[e].y;
        float distance = sqrt(dx * dx + dy * dy);
        if (distance < 1.0f) {
            enemy->RemoveBot01(e);
            player->TakeDamage(40.0f);
            if (explosionCooldownTimer <= 0.0f) {
                sfxExplosion->Play(sfxPlayer);
                explosionCooldownTimer = explosionCooldown;
            }
        }
    }
}

// Render
void SceneMuntasir::Render() const {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

    // Player color CYAN
    glUniform4f(shader->GetUniformID("color"), 0.0f, 1.0f, 1.0f, 1.0f);
    player->Render(shader, projectionMatrix, viewMatrix);

    // Bullets color YELLOW
    glUniform4f(shader->GetUniformID("color"), 1.0f, 1.0f, 0.0f, 1.0f);
    bullet->Render(shader, projectionMatrix, viewMatrix);

    // Enemies color ORANGE
    glUniform4f(shader->GetUniformID("color"), 1.0f, 0.3f, 0.0f, 1.0f);
    enemy->Render(shader, projectionMatrix, viewMatrix);

    glUseProgram(0);

    // Starfield - drawn after OpenGL
    environment->Render();
}

// DrawGui
void SceneMuntasir::DrawGui() {

    // HUD
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300, 260), ImGuiCond_Always);
    ImGui::Begin("Alpha Wing EX", nullptr, ImGuiWindowFlags_NoResize);

    // Score
    ImGui::Text("SCORE: %d", score);

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
        if (ImGui::Button("Play Music", ImVec2(120, 30))) {
            SDL_ResumeAudioStreamDevice(audioPlayer);
            musicPaused = false;
        }
    }
    else {
        if (ImGui::Button("Pause Music", ImVec2(120, 30))) {
            SDL_PauseAudioStreamDevice(audioPlayer);
            musicPaused = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Exit Game", ImVec2(120, 30))) {
        SDL_Event quitEvent;
        quitEvent.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quitEvent);
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
        if (ImGui::Button("Try Again", ImVec2(130, 40))) {
            gameOver = false;
            score = 0;
            player->Reset();
            enemy->Reset();
        }
        ImGui::SameLine();
        if (ImGui::Button("Exit Game", ImVec2(130, 40))) {
            SDL_Event quitEvent;
            quitEvent.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&quitEvent);
        }
        ImGui::End();
    }
}