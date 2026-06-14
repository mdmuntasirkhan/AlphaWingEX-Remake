#include <SDL.h>
#include "SceneManager.h"
#include "Timer.h"
#include "Window.h"
#include "SceneSTG.h"
#include "SceneTestJacky.h"
#include "SceneMuntasir.h"



// ImGui
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"







SceneManager::SceneManager(): 
	currentScene{nullptr}, window{nullptr}, timer{nullptr},
	fps(60), isRunning{false}, fullScreen{false} {
	Debug::Info("Starting the SceneManager", __FILE__, __LINE__);
}

SceneManager::~SceneManager() {
	Debug::Info("Deleting the SceneManager", __FILE__, __LINE__);

	if (currentScene) {
		currentScene->OnDestroy();
		delete currentScene;
		currentScene = nullptr;
	}
	
	if (timer) {
		delete timer;
		timer = nullptr;
	}

	if (window) {
		delete window;
		window = nullptr;
	}
	
}

bool SceneManager::Initialize(std::string name_, int width_, int height_) {

	window = new Window();
	if (!window->OnCreate(name_, width_, height_)) {
		Debug::FatalError("Failed to initialize Window object", __FILE__, __LINE__);
		return false;
	}

	timer = new Timer();
	if (timer == nullptr) {
		Debug::FatalError("Failed to initialize Timer object", __FILE__, __LINE__);
		return false;
	}

	/// imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplSDL3_InitForOpenGL(window->getWindow(), SDL_GL_GetCurrentContext());
	ImGui_ImplOpenGL3_Init("#version 450");


	/********************************   Default first scene   ***********************/
	BuildNewScene(SCENE_NUMBER::SCENEJA);
	/********************************************************************************/
	return true;
}

/// This is the whole game
void SceneManager::Run() {
	timer->Start();
	isRunning = true;
	while (isRunning) {
		HandleEvents();
		timer->UpdateFrameTicks();



		// Start ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();


		currentScene->Update(timer->GetDeltaTime());
		currentScene->Render();

		currentScene->DrawGui();

		// Render ImGui
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		
		SDL_GL_SwapWindow(window->getWindow());
		SDL_Delay(timer->GetSleepTime(fps));
	}
}

void SceneManager::HandleEvents() {
	SDL_Event sdlEvent;
	while (SDL_PollEvent(&sdlEvent)) { /// Loop over all events in the SDL queue

		// ImGui gets events FIRST - this is what was missing!
		ImGui_ImplSDL3_ProcessEvent(&sdlEvent);

		if (sdlEvent.type == SDL_EventType::SDL_EVENT_QUIT) {
			isRunning = false;
			return;
		}
		else if (sdlEvent.type == SDL_EVENT_KEY_DOWN) {
			switch (sdlEvent.key.scancode) {
			[[fallthrough]]; /// C17 Prevents switch/case fallthrough warnings
			case SDL_SCANCODE_ESCAPE:
			case SDL_SCANCODE_Q:
				isRunning = false;
				return;

			case SDL_SCANCODE_F1:
				// switch to scene0 default
				BuildNewScene(SCENE_NUMBER::SCENESTG);
				break;

			case SDL_SCANCODE_F2:
				// switch to scene0 default
				BuildNewScene(SCENE_NUMBER::SCENEJA);
				break;

			case SDL_SCANCODE_F3:
				// switch to scene0 default
				BuildNewScene(SCENE_NUMBER::SCENEMUN);
				break;

			case SDL_SCANCODE_F6:
				break;
			default:
				break;
			}
		}
		if (currentScene == nullptr) { /// Just to be careful
			Debug::FatalError("No currentScene", __FILE__, __LINE__);
			isRunning = false;
			return;
		}
		currentScene->HandleEvents(sdlEvent);
	}
}

bool SceneManager::BuildNewScene(SCENE_NUMBER scene) {
	bool status; 

	if (currentScene != nullptr) {
		currentScene->OnDestroy();
		delete currentScene;
		currentScene = nullptr;
	}

	switch (scene) {
	// default scenes
	case SCENE_NUMBER::SCENESTG:
		currentScene = new SceneSTG();
		status = currentScene->OnCreate();
		break;

	case SCENE_NUMBER::SCENEJA:
		currentScene = new SceneJA();
		status = currentScene->OnCreate();
		break;

	case SCENE_NUMBER::SCENEMUN:
		currentScene = new SceneMuntasir();
		status = currentScene->OnCreate();
		break;

	default:
		Debug::Error("Incorrect scene number assigned in the manager", __FILE__, __LINE__);
		currentScene = nullptr;
		return false;
	}

	return true;
}


