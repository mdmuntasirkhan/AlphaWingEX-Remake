#include <glew.h>
#include <iostream>
#include <SDL.h>
#include <SDL3/SDL_events.h>
#include "SceneMuntasir.h"
#include <MMath.h>
#include "Debug.h"
#include "Mesh.h"
#include "Shader.h"
#include "Body.h"
#include "Texture.h"
#include <QMath.h>
#include "Player.h"
#include "imgui.h"

// Constructor
SceneMuntasir::SceneMuntasir() :
    player{ nullptr },
    shader{ nullptr },
    drawInWireMode{ false },
    bulletSpeed{ 10.0f },
    bulletMesh{ nullptr },
    Bot01Mesh{ nullptr },
    Bot01Speed{ 3.0f },
    spawnTimer{ 0.0f },
    spawnInterval{ 2.0f },
    asteroidMesh{ nullptr },
    asteroidSpeed{ 2.0f },
    asteroidSpawnTimer{ 0.0f },
    asteroidSpawnInterval{ 1.5f },
    gameOver{ false },
    score{ 0 },
    audioPlayer{ nullptr },
    sfxPlayer{ nullptr },
    sfxLaser{ nullptr },
    sfxExplosion{ nullptr },
    audioTest{ nullptr },
    musicVolume{ 0.5f },
    sfxVolume{ 1.0f },
    musicPaused{ false },
    explosionCooldown{ 0.2f },
    explosionCooldownTimer{ 0.0f },
    environment{ nullptr } {
    Debug::Info("Created SceneMuntasir: ", __FILE__, __LINE__);
}

// Destructor
SceneMuntasir::~SceneMuntasir() {
    Debug::Info("Deleted SceneMuntasir: ", __FILE__, __LINE__);
}

// OnCreate
bool SceneMuntasir::OnCreate() {
    Debug::Info("Loading assets SceneMuntasir: ", __FILE__, __LINE__);

    // Player - uses Player class
    player = new Player();
    if (player->OnCreate("meshes/Temp_AlphaWingEX.obj") == false) {
        std::cout << "Player failed to load!\n";
        return false;
    }

    // Bullet Mesh
    bulletMesh = new Mesh("meshes/Temp_AlphaWing_Bullet.obj");
    if (bulletMesh->OnCreate() == false) {
        std::cout << "Bullet Mesh not found!\n";
        return false;
    }

    // Asteroid Mesh
    asteroidMesh = new Mesh("meshes/Temp_Asteroid.obj");
    if (asteroidMesh->OnCreate() == false) {
        std::cout << "Asteroid mesh not found!\n";
        return false;
    }

    // Enemy Bot01 Mesh
    Bot01Mesh = new Mesh("meshes/Temp_AlphaWing_Enemy_Bot01.obj");
    if (Bot01Mesh->OnCreate() == false) {
        std::cout << "Enemy mesh not found!\n";
        return false;
    }

    // Environment
    environment = new Environment();
    if (environment->OnCreate(1920.0f, 1080.0f) == false) {
        return false;
    }

    // Load shader
    shader = new Shader("shaders/defaultVert.glsl", "shaders/defaultFrag.glsl");
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

    sfxExplosion = new Sound("audio/sfx/Explosion01.wav");
    sfxExplosion->OnCreate();

    SDL_Log("Music queued bytes: %d", SDL_GetAudioStreamQueued(audioPlayer));
    SDL_Log("SFX queued bytes: %d", SDL_GetAudioStreamQueued(sfxPlayer));

    return true;
}

// OnDestroy
void SceneMuntasir::OnDestroy() {
    Debug::Info("Deleting assets SceneMuntasir: ", __FILE__, __LINE__);

    // Player
    player->OnDestroy();
    delete player;
    player = nullptr;

    // Bullet
    bulletMesh->OnDestroy();
    delete bulletMesh;
    bulletPositions.clear();

    // Asteroid
    asteroidMesh->OnDestroy();
    delete asteroidMesh;
    asteroidPositions.clear();

    // Bot01
    Bot01Mesh->OnDestroy();
    delete Bot01Mesh;
    Bot01Positions.clear();

    // Environment
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
            bulletPositions.push_back(player->GetPosition());
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
                bulletPositions.push_back(player->GetPosition());
                sfxLaser->Play(sfxPlayer);
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

    // Player updates itself
    player->Update(deltaTime);

    // Check game over through player
    if (player->IsGameOver()) {
        gameOver = true;
    }

    // Move bullets forward
    for (int i = 0; i < bulletPositions.size(); i++) {
        bulletPositions[i].x += bulletSpeed * deltaTime;
    }

    // Remove bullets off screen
    for (int i = bulletPositions.size() - 1; i > 0; i--) {
        if (bulletPositions[i].x > 15.0f) {
            bulletPositions.erase(bulletPositions.begin() + i);
        }
    }

    // Asteroid spawn - wave 1
    asteroidSpawnTimer += deltaTime;
    if (asteroidSpawnTimer >= asteroidSpawnInterval) {
        asteroidSpawnTimer = 0.0f;
        float randomY = ((rand() % 10) - 5) * 0.5f;
        asteroidPositions.push_back(Vec3(15.0f, randomY, -10.0f));
    }

    // Move asteroids
    for (int i = 0; i < asteroidPositions.size(); i++) {
        asteroidPositions[i].x -= asteroidSpeed * deltaTime;
    }

    // Remove asteroids off screen
    for (int i = asteroidPositions.size() - 1; i >= 0; i--) {
        if (asteroidPositions[i].x < -15.0f) {
            asteroidPositions.erase(asteroidPositions.begin() + i);
        }
    }

    // Bot01 spawn - wave 2 after 30 seconds
    if (totalTime > 30.0f) {
        spawnTimer += deltaTime;
        if (spawnTimer >= spawnInterval) {
            spawnTimer = 0.0f;
            float randomY = ((rand() % 10) - 5) * 0.5f;
            Bot01Positions.push_back(Vec3(15.0f, randomY, -10.0f));
        }
    }

    // Move Bot01
    for (int i = 0; i < Bot01Positions.size(); i++) {
        Bot01Positions[i].x -= Bot01Speed * deltaTime;
    }

    // Remove Bot01 off screen
    for (int i = Bot01Positions.size() - 1; i >= 0; i--) {
        if (Bot01Positions[i].x < -15.0f) {
            Bot01Positions.erase(Bot01Positions.begin() + i);
        }
    }

    // Environment
    environment->Update(deltaTime);

    // Collision - bullet hits Bot01
    for (int b = bulletPositions.size() - 1; b >= 0; b--) {
        for (int e = Bot01Positions.size() - 1; e >= 0; e--) {
            float dx = bulletPositions[b].x - Bot01Positions[e].x;
            float dy = bulletPositions[b].y - Bot01Positions[e].y;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < 1.0f) {
                bulletPositions.erase(bulletPositions.begin() + b);
                Bot01Positions.erase(Bot01Positions.begin() + e);
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
    for (int b = bulletPositions.size() - 1; b >= 0; b--) {
        for (int a = asteroidPositions.size() - 1; a >= 0; a--) {
            float dx = bulletPositions[b].x - asteroidPositions[a].x;
            float dy = bulletPositions[b].y - asteroidPositions[a].y;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < 1.0f) {
                bulletPositions.erase(bulletPositions.begin() + b);
                asteroidPositions.erase(asteroidPositions.begin() + a);
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
    for (int a = asteroidPositions.size() - 1; a >= 0; a--) {
        float dx = player->GetPosition().x - asteroidPositions[a].x;
        float dy = player->GetPosition().y - asteroidPositions[a].y;
        float distance = sqrt(dx * dx + dy * dy);
        if (distance < 1.0f) {
            asteroidPositions.erase(asteroidPositions.begin() + a);
            player->TakeDamage(25.0f);
            if (explosionCooldownTimer <= 0.0f) {
                sfxExplosion->Play(sfxPlayer);
                explosionCooldownTimer = explosionCooldown;
            }
        }
    }

    // Collision - Bot01 hits player
    for (int e = Bot01Positions.size() - 1; e >= 0; e--) {
        float dx = player->GetPosition().x - Bot01Positions[e].x;
        float dy = player->GetPosition().y - Bot01Positions[e].y;
        float distance = sqrt(dx * dx + dy * dy);
        if (distance < 1.0f) {
            Bot01Positions.erase(Bot01Positions.begin() + e);
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

    if (drawInWireMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glUseProgram(shader->GetProgram());

    // Player renders itself
    player->Render(shader, projectionMatrix, viewMatrix);

    // Bullets
    for (int i = 0; i < bulletPositions.size(); i++) {
        Matrix4 bulletMatrix = MMath::translate(bulletPositions[i]) *
            MMath::scale(0.2f, 0.2f, 0.2f);
        glUniformMatrix4fv(shader->GetUniformID("modelMatrix"),
            1, GL_FALSE, bulletMatrix);
        bulletMesh->Render();
    }

    // Asteroids
    for (int i = 0; i < asteroidPositions.size(); i++) {
        Matrix4 asteroidMatrix = MMath::translate(asteroidPositions[i]) *
            MMath::scale(0.4f, 0.4f, 0.4f);
        glUniformMatrix4fv(shader->GetUniformID("modelMatrix"),
            1, GL_FALSE, asteroidMatrix);
        asteroidMesh->Render();
    }

    // Bot01
    for (int i = 0; i < Bot01Positions.size(); i++) {
        Matrix4 enemyMatrix = MMath::translate(Bot01Positions[i]) *
            MMath::rotate(180.0f, Vec3(0.0f, 1.0f, 0.0f)) *
            MMath::scale(0.3f, 0.3f, 0.3f);
        glUniformMatrix4fv(shader->GetUniformID("modelMatrix"),
            1, GL_FALSE, enemyMatrix);
        Bot01Mesh->Render();
    }

    // Environment
    environment->Render();

    glUseProgram(0);
}

// DrawGui
void SceneMuntasir::DrawGui() {

    // Audio Control Panel
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

    // Shield status
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

    // Exit
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
            bulletPositions.clear();
            asteroidPositions.clear();
            Bot01Positions.clear();
            player->Reset();
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