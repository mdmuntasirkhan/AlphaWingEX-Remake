#include <glew.h>
#include <iostream>
#include <SDL.h>
#include <SDL3/SDL_events.h>
#include "SceneTestJacky.h"
#include <MMath.h>
#include "Debug.h"
#include "Mesh.h"
#include "Shader.h"
#include "Body.h"
#include "Texture.h"
#include <QMath.h>
#include "Camera.h"

SceneJA::SceneJA() :testOBJ{ nullptr }, shader{ nullptr }, mesh{ nullptr }, soundManager(nullptr),
drawInWireMode{ false } {
	Debug::Info("Created SceneJA: ", __FILE__, __LINE__);
}

SceneJA::~SceneJA() {
	Debug::Info("Deleted SceneJA: ", __FILE__, __LINE__);
}

bool SceneJA::OnCreate() {
	Debug::Info("Loading assets SceneJA: ", __FILE__, __LINE__);

	testOBJ = new Body();
	testOBJ->OnCreate();

	shader = new Shader("shaders/defaultVert.glsl", "shaders/defaultFrag.glsl");
	if (shader->OnCreate() == false) {
		std::cout << "Shader failed ... we have a problem\n";
	}

	testOBJ->setMesh("meshes/Temp_AlphaWingEX.obj");

	camera = new Camera();
	camera->OnCreate();

	soundManager = new SoundManager();
	soundManager->OnCreate();

	sound2 = new Sound("audio/273711__sfx4animation__whoosh-sfx-40.wav");
	sound2->OnCreate();

	soundManager->adjustSFXVolume(0.3f);

	return true;
}

void SceneJA::OnDestroy() {
	Debug::Info("Deleting assets SceneJA: ", __FILE__, __LINE__);
	if (testOBJ) {
		testOBJ->OnDestroy();
		delete testOBJ;
	}

	if (shader) {
		shader->OnDestroy();
		delete shader;
	}

	if (camera) {
		camera->OnDestroy();
		delete camera;
	}

	if (soundManager) {
		soundManager->OnDestroy();
		delete soundManager;
	}

	if (sound1) {
		sound1->OnDestroy();
		delete sound1;
	}

	if (sound2) {
		sound2->OnDestroy();
		delete sound2;
	}
}

void SceneJA::HandleEvents(const SDL_Event& sdlEvent) {
	switch (sdlEvent.type) {
	case SDL_EVENT_KEY_DOWN:
		switch (sdlEvent.key.scancode) {
		case SDL_SCANCODE_W:
			drawInWireMode = !drawInWireMode;
			break;
		case SDL_SCANCODE_F:
			soundManager->playSoundAt(sound2);
			break;
		case SDL_SCANCODE_G:
			soundManager->adjustMasterVolume(0.6f);
			break;
		default:
			break;
		}
		break;
	}
}

void SceneJA::Update(const float deltaTime) {
	static float totalTime = 0.0f;
	totalTime += deltaTime;

}

void SceneJA::Render() const {
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
	glUniformMatrix4fv(shader->GetUniformID("projectionMatrix"), 1, GL_FALSE, camera->getProjectionMatrix());
	glUniformMatrix4fv(shader->GetUniformID("viewMatrix"), 1, GL_FALSE, camera->getViewMatrix());
	testOBJ->Render(shader);

	glUseProgram(0); // TURN OFF THE SHADER
}
