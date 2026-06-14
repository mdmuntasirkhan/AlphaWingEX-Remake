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

// imgui
#include "imgui.h"

// Constructor
SceneMuntasir::SceneMuntasir() : playerShip{ nullptr }, shader{ nullptr }, AlphaWingMesh{ nullptr }, audioPlayer{ nullptr },
drawInWireMode{ false }, playerSpeed{ 5.0f }, bulletSpeed{ 10.0f }, bulletMesh{ nullptr }, Bot01Mesh{ nullptr }, 
Bot01Speed{ 3.0f }, spawnTimer{ 0.0f }, spawnInterval{ 2.0f }, musicVolume{ 0.5f }, sfxVolume{ 1.0f }, musicPaused{ false },
score{ 0 }, lives{ 3 }, health{ 100 }, maxHealth{ 100 }, asteroidMesh{ nullptr }, asteroidSpeed{ 2.0f }, asteroidSpawnTimer{ 0.0f }, 
asteroidSpawnInterval{ 1.5f }, gameOver{ false } {
	Debug::Info("Created SceneMuntasir: ", __FILE__, __LINE__);
}

// Destructor
SceneMuntasir::~SceneMuntasir() {
	Debug::Info("Deleted SceneMuntasir: ", __FILE__, __LINE__);
}

// OnCreate
bool SceneMuntasir::OnCreate() {
	Debug::Info("Loading assets SceneMuntasir: ", __FILE__, __LINE__);

	// Load Alpha wing Model
	AlphaWingMesh = new Mesh("meshes/Temp_AlphaWingEX.obj");
	if (AlphaWingMesh->OnCreate() == false) {
		std::cout << "Mesh faild to load!\n";
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
		std::cout << "Enemy mesh not in there!\n";
		return false;
	}

	// Create the body with physics 
	playerShip = new Body();
	playerShip->OnCreate();

	// Load shader
	shader = new Shader("shaders/defaultVert.glsl", "shaders/defaultFrag.glsl");
	if (shader->OnCreate() == false) {
		std::cout << "Shader failed!\n";
		return false;
	}

	// Player Position
	playerPos = Vec3(0.0f, 0.0f, -10.0f);

	// Placement of Alpha Wing
	playerModelMatrix = MMath::translate(playerPos);

	// Camera looking forward
	viewMatrix = MMath::lookAt(
		Vec3(0.0f, 0.0f, 0.0f),
		Vec3(0.0f, 0.0f, -1.0f),
		Vec3(0.0f, 1.0f, 0.0f)
	);

	// Perspective (this belongs to projectionMatrix, not playerModelMatrix)
	projectionMatrix = MMath::perspective(45.0f, 16.0f / 9.0f, 0.1f, 100.0f);

	// Audio Setup
	SDL_AudioSpec defaultSpec;
	defaultSpec.freq = 44100;
	defaultSpec.channels = 2;
	defaultSpec.format = SDL_AUDIO_S16;

	// Music stream
	audioPlayer = SDL_OpenAudioDeviceStream(
		SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
		&defaultSpec,
		nullptr,
		nullptr
	);
	if (!audioPlayer) {
		std::cout << "Failed to create music player\n";
	}
	SDL_ResumeAudioStreamDevice(audioPlayer);

	// SFX stream
	sfxPlayer = SDL_OpenAudioDeviceStream(
		SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
		&defaultSpec,
		nullptr,
		nullptr
	);
	if (!sfxPlayer) {
		std::cout << "Failed to create SFX player\n";
	}
	SDL_ResumeAudioStreamDevice(sfxPlayer);

	// Load and play music on audioPlayer
	audioTest = new Sound("audio/music/deadmou5-gg.wav");
	audioTest->OnCreate();
	audioTest->Play(audioPlayer);

	// Load SFX - plays on sfxPlayer
	sfxLaser = new Sound("audio/sfx/LaserShoot.wav");
	sfxLaser->OnCreate();

	// SFX Explosion
	sfxExplosion = new Sound("audio/sfx/Explosion01.wav");
	sfxExplosion->OnCreate();

	// Log both streams
	SDL_Log("Music queued bytes: %d", SDL_GetAudioStreamQueued(audioPlayer));
	SDL_Log("SFX queued bytes: %d", SDL_GetAudioStreamQueued(sfxPlayer));

	return true;
}

// OnDestroy
void SceneMuntasir::OnDestroy() {
	//Debug
	Debug::Info("Deleting assets SceneMuntasir: ", __FILE__, __LINE__);

	// Player Ship
	playerShip->OnDestroy();
	delete playerShip;

	// Alpha Wing Mesh
	AlphaWingMesh->OnDestroy();
	delete AlphaWingMesh;

	// Bullet Mesh
	bulletMesh->OnDestroy();
	delete bulletMesh;
	bulletPositions.clear();

	// Asteroid Mesh
	asteroidMesh->OnDestroy();
	delete asteroidMesh;
	asteroidPositions.clear();

	// Bot01 Mesh
	Bot01Mesh->OnDestroy();
	delete Bot01Mesh;
	Bot01Positions.clear();

	// Shader
	shader->OnDestroy();
	delete shader;

	// SFX Laser
	sfxLaser->OnDestroy();
	delete sfxLaser;

	// SFX Explosion
	sfxExplosion->OnDestroy();
	delete sfxExplosion;

	// Audio player
	SDL_DestroyAudioStream(audioPlayer);
	SDL_DestroyAudioStream(sfxPlayer);
	audioTest->OnDestroy();
	delete audioTest;
}

// HandleEvents
void SceneMuntasir::HandleEvents(const SDL_Event& sdlEvent) {
	// Keyboard
	switch (sdlEvent.type) {
	case SDL_EVENT_KEY_DOWN:
		switch (sdlEvent.key.scancode) {
		case SDL_SCANCODE_F12:
			drawInWireMode = !drawInWireMode;
			break;
		case SDL_SCANCODE_W:
			playerPos.y += playerSpeed * 0.1f;
			if (playerPos.y > 4.5f) playerPos.y = 4.5f;
			break;
		case SDL_SCANCODE_S:
			playerPos.y -= playerSpeed * 0.1f;
			if (playerPos.y < -4.5f) playerPos.y = -4.5f;
			break;
		case SDL_SCANCODE_A:
			playerPos.x -= playerSpeed * 0.1f;
			if (playerPos.x < -8.0f) playerPos.x = -8.0f;
			break;
		case SDL_SCANCODE_D:
			playerPos.x += playerSpeed * 0.1f;
			if (playerPos.x > 8.0f) playerPos.x = 8.0f;
			break;
		case SDL_SCANCODE_SPACE:
			bulletPositions.push_back(playerPos);
			sfxLaser->Play(sfxPlayer);
			break;
		default:
			break;
		}
		break;

	// Mouse
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
		if (sdlEvent.button.button == SDL_BUTTON_LEFT) {
			// Only shoot if ImGui is NOT using the mouse
			if (!ImGui::GetIO().WantCaptureMouse) {
				bulletPositions.push_back(playerPos);
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

	// If game over, stop updating everything
	if (gameOver) return;

	// Player Matrix
	playerModelMatrix = MMath::translate(playerPos) *
						MMath::scale(0.3f, 0.3f, 0.3f);

	// Move every bullet forward
	for (int i = 0; i < bulletPositions.size(); i++) {
		bulletPositions[i].x += bulletSpeed * deltaTime;
	}

	// Remove bullets that have gone off screens
	for (int i = bulletPositions.size() - 1; i > +0; i--) {
		if (bulletPositions[i].x > 15.0f) {
			bulletPositions.erase(bulletPositions.begin() + i);
		}
	}

	// Asteroid spawn timer - wave 1
	asteroidSpawnTimer += deltaTime;
	if (asteroidSpawnTimer >= asteroidSpawnInterval) {
		asteroidSpawnTimer = 0.0f;
		float randomY = ((rand() % 10) - 5) * 0.5f;
		asteroidPositions.push_back(Vec3(15.0f, randomY, -10.0f));
	}

	// Move asteroids left
	for (int i = 0; i < asteroidPositions.size(); i++) {
		asteroidPositions[i].x -= asteroidSpeed * deltaTime;
	}

	// Remove asteroid off the screen
	for (int i = asteroidPositions.size() - 1; i >= 0; i--) {
		if (asteroidPositions[i].x < -15.0f) {
			asteroidPositions.erase(asteroidPositions.begin() + i);
		}
	}



	// Bot01 Spawn timer
	spawnTimer += deltaTime;
	if (spawnTimer >= spawnInterval) {

		spawnTimer = 0.0f;

		// Spawn Bot01 on the right side, random Y height
		float randomY = ((rand() % 10) - 5) * 0.5f;
		Bot01Positions.push_back(Vec3(15.0f, randomY, -10.0f));
	}

	// Move Bot01 to the left towards player
	for (int i = 0; i < Bot01Positions.size(); i++) {
		Bot01Positions[i].x -= Bot01Speed * deltaTime;
	}

	// Remove enemies that passed player
	for (int i = Bot01Positions.size() - 1; i >= 0; i--) {
		if (Bot01Positions[i].x < -15.0f) {
			Bot01Positions.erase(Bot01Positions.begin() + i);
		}
	}

	//// Collision - bullet hits enemy
	//for (int b = bulletPositions.size() - 1; b >= 0; b--) {
	//	for (int e = Bot01Positions.size() - 1; e >= 0; e--) {

	//		// Get Distance between bullet and the enemy
	//		float dx = bulletPositions[b].x - Bot01Positions[e].x;
	//		float dy = bulletPositions[b].y - Bot01Positions[e].y;
	//		float distance = sqrt(dx * dx + dy * dy);

	//		// If close enough - hit!
	//		if (distance < 1.0f) {
	//			bulletPositions.erase(bulletPositions.begin() + b);
	//			Bot01Positions.erase(Bot01Positions.begin() + e);
	//			score += 100;
	//			sfxExplosion->Play(sfxPlayer);
	//			break;
	//		}
	//	}
	//}

	// Collision - bullet hits enemy
	for (int b = bulletPositions.size() - 1; b >= 0; b--) {
		for (int e = Bot01Positions.size() - 1; e >= 0; e--) {
			float dx = bulletPositions[b].x - Bot01Positions[e].x;
			float dy = bulletPositions[b].y - Bot01Positions[e].y;
			float distance = sqrt(dx * dx + dy * dy);
			if (distance < 1.0f) {
				bulletPositions.erase(bulletPositions.begin() + b);
				Bot01Positions.erase(Bot01Positions.begin() + e);
				score += 100;
				sfxExplosion->Play(sfxPlayer);
				break;
			}
		}
	}

	// Collision - bullet hits asteroid (same idea, less points)
	for (int b = bulletPositions.size() - 1; b >= 0; b--) {
		for (int a = asteroidPositions.size() - 1; a >= 0; a--) {
			float dx = bulletPositions[b].x - asteroidPositions[a].x;
			float dy = bulletPositions[b].y - asteroidPositions[a].y;
			float distance = sqrt(dx * dx + dy * dy);
			if (distance < 1.0f) {
				bulletPositions.erase(bulletPositions.begin() + b);
				asteroidPositions.erase(asteroidPositions.begin() + a);
				score += 50;                    // asteroids worth less
				sfxExplosion->Play(sfxPlayer);
				break;
			}
		}
	}

	// Collision - asteroid hits player
	for (int a = asteroidPositions.size() - 1; a >= 0; a--) {
		float dx = playerPos.x - asteroidPositions[a].x;
		float dy = playerPos.y - asteroidPositions[a].y;
		float distance = sqrt(dx * dx + dy * dy);
		if (distance < 1.0f) {
			asteroidPositions.erase(asteroidPositions.begin() + a);
			health -= 25.0f;
			sfxExplosion->Play(sfxPlayer);
			if (health <= 0.0f) {
				lives--;
				health = maxHealth;
			}
		}
	}

	// Collision - Bot01 hits player
	for (int e = Bot01Positions.size() - 1; e >= 0; e--) {
		float dx = playerPos.x - Bot01Positions[e].x;
		float dy = playerPos.y - Bot01Positions[e].y;
		float distance = sqrt(dx * dx + dy * dy);
		if (distance < 1.0f) {
			Bot01Positions.erase(Bot01Positions.begin() + e);
			health -= 40.0f;                // enemies hurt more than asteroids
			sfxExplosion->Play(sfxPlayer);
			if (health <= 0.0f) {
				lives--;
				health = maxHealth;
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

	glUseProgram(shader->GetProgram()); // turn on shader
	glUniformMatrix4fv(shader->GetUniformID("projectionMatrix"), 1, GL_FALSE, projectionMatrix);
	glUniformMatrix4fv(shader->GetUniformID("viewMatrix"), 1, GL_FALSE, viewMatrix);
	glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, playerModelMatrix);

	// Draw Alpha WIng
	AlphaWingMesh->Render();

	// Draw Every Bullet
	for (int i = 0; i < bulletPositions.size(); i++) {
		Matrix4 bulletMatrix = MMath::translate(bulletPositions[i]);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, bulletMatrix);
		bulletMesh->Render();
	}

	// Draw Asteroids
	for (int i = 0; i < asteroidPositions.size(); i++) {
		Matrix4 asteroidMatrix = MMath::translate(asteroidPositions[i]) *
			MMath::scale(0.4f, 0.4f, 0.4f);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, asteroidMatrix);
		asteroidMesh->Render();
	}

	// Draw Bot01
	for (int i = 0; i < Bot01Positions.size(); i++) {
		Matrix4 enemyMatrix = MMath::translate(Bot01Positions[i]) *
							  MMath::rotate(180.0f, Vec3(0.0f, 1.0f, 0.0f)) *
							  MMath::scale(0.3f, 0.3f, 0.3f);
		glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, enemyMatrix);
		Bot01Mesh->Render();
	}

	glUseProgram(0); // TURN OFF THE SHADER
}

// imgui
void SceneMuntasir::DrawGui() {

	// Audio Control Panel
	ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(300, 160), ImGuiCond_Always);
	ImGui::Begin("Audio Settings", nullptr, ImGuiWindowFlags_NoResize);

	// Score
	ImGui::Text("SCORE: %d", score);

	ImGui::Text("LIVES: ");
	ImGui::SameLine();
	for (int i = 0; i < lives; i++) {
		ImGui::Text("[*]");
		ImGui::SameLine();
	}
	ImGui::NewLine();

	// Health bar changes color green to yellow to red
	float healthPercent = health / maxHealth;
	if (healthPercent > 0.6f) {
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
	}
	else if (healthPercent > 0.3f) {
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
	}
	else {
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
	}
	ImGui::ProgressBar(healthPercent, ImVec2(260, 20), "");
	ImGui::PopStyleColor();



	// Music volume slider
	ImGui::Text("Music Volume");
	if (ImGui::SliderFloat("##music", &musicVolume, 0.0f, 1.0f)) {
		// Apply music volume
		SDL_SetAudioStreamGain(audioPlayer, musicVolume);
	}

	// SFX volume slider
	ImGui::Text("SFX Volume");
	if (ImGui::SliderFloat("##sfx", &sfxVolume, 0.0f, 1.0f)) {
		// Apply SFX volume when slider moves
		SDL_SetAudioStreamGain(sfxPlayer, sfxVolume);
	}

	// Play/Pause button
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

	ImGui::SameLine(); // put next button on same line

	// Exit button
	if (ImGui::Button("Exit Game", ImVec2(120, 30))) {
		SDL_Event quitEvent;
		quitEvent.type = SDL_EVENT_QUIT;
		SDL_PushEvent(&quitEvent);
	}

	ImGui::End();
}