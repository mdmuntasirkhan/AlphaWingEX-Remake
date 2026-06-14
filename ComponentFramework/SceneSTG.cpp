#include <glew.h>
#include <iostream>
#include <SDL.h>
#include <SDL3/SDL_events.h>
#include "SceneSTG.h"
#include <MMath.h>
#include "Debug.h"
#include "Mesh.h"
#include "Shader.h"
#include "Body.h"
#include "Texture.h"
#include <QMath.h>


// imgui
#include "imgui.h"

SceneSTG::SceneSTG() :sphere{ nullptr }, shader{ nullptr }, mesh{ nullptr }, audioPlayer{ nullptr },
drawInWireMode{ false } {
	Debug::Info("Created Scene0: ", __FILE__, __LINE__);
}

SceneSTG::~SceneSTG() {
	Debug::Info("Deleted Scene0: ", __FILE__, __LINE__);
}

bool SceneSTG::OnCreate() {
	Debug::Info("Loading assets SceneSTG: ", __FILE__, __LINE__);

	sphere = new Body();
	sphere->OnCreate();

	shader = new Shader("shaders/defaultVert.glsl", "shaders/defaultFrag.glsl");
	if (shader->OnCreate() == false) {
		std::cout << "Shader failed ... we have a problem\n";
	}

	//mesh = new Mesh("meshes/Sphere.obj");
	//mesh->OnCreate();
	
	SDL_AudioSpec defaultSpec;
	defaultSpec.freq = 48000;
	defaultSpec.channels = 2;
	defaultSpec.format = SDL_AUDIO_S16;

	audioPlayer = SDL_OpenAudioDeviceStream(
		SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
		&defaultSpec,
		nullptr,
		nullptr
	);

	if (!audioPlayer) {
		std::cout << "Failed to create audio player\n";
	}

	SDL_ResumeAudioStreamDevice(audioPlayer);

	audioTest = new Sound("audio/Sabbat.wav");
	audioTest->OnCreate();

	audioTest->Play(audioPlayer);

	int queued = SDL_GetAudioStreamQueued(audioPlayer);

	SDL_Log("Queued bytes: %d", queued);

	return true;
}

void SceneSTG::OnDestroy() {
	Debug::Info("Deleting assets Scene0: ", __FILE__, __LINE__);
	sphere->OnDestroy();
	delete sphere;

	//mesh->OnDestroy();
	//delete mesh;

	shader->OnDestroy();
	delete shader;

	// destroy audio player
	SDL_DestroyAudioStream(audioPlayer);

	audioTest->OnDestroy();
	delete audioTest;
}

void SceneSTG::HandleEvents(const SDL_Event& sdlEvent) {
	switch (sdlEvent.type) {
	case SDL_EVENT_KEY_DOWN:
		switch (sdlEvent.key.scancode) {
		case SDL_SCANCODE_W:
			drawInWireMode = !drawInWireMode;
			break;
		default:
			break;
		}
		break;
	}
}

void SceneSTG::Update(const float deltaTime) {
	static float totalTime = 0.0f;
	totalTime += deltaTime;

}

void SceneSTG::Render() const {
	glEnable(GL_DEPTH_TEST);

	/// Set the background color then clear the screen
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
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

	glUseProgram(0); // TURN OFF THE SHADER
}





/// imgui
void SceneSTG::DrawGui() {
	// Optional tiny debug window
	ImGui::Begin("Scene3p Debug");
	ImColor textColor(255, 255, 255);
	ImGui::Text("Yay, ImGui is working!");
	ImGui::End();

	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* drawList = ImGui::GetForegroundDrawList();

	// Center of screen
	ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);

	// Crosshair settings
	float crosshairSize = 20.0f;
	float crosshairThickness = 5.0f;

	// Horizontal line
	drawList->AddLine(
		ImVec2(center.x - crosshairSize, center.y),
		ImVec2(center.x + crosshairSize, center.y),
		IM_COL32(255, 255, 255, 255),
		crosshairThickness
	);

	// Vertical line
	drawList->AddLine(
		ImVec2(center.x, center.y - crosshairSize),
		ImVec2(center.x, center.y + crosshairSize),
		IM_COL32(255, 255, 255, 255),
		crosshairThickness
	);
}