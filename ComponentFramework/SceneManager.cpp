#include <SDL.h>
#include "SceneManager.h"
#include "SaveData.h"
#include "Timer.h"
#include "Window.h"
#include "SceneSTG.h"
#include "SceneTestJacky.h"
#include "SceneMuntasir.h"
#include "SceneTitle.h"
#include "SceneSwitcher.h"



// ImGui
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"







SceneManager::SceneManager():
	currentScene{nullptr}, window{nullptr}, timer{nullptr},
	fps(60), isRunning{false}, fullScreen{false}, vsyncActive{false} {
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

	// Restore machine-level video/audio prefs before the first scene starts.
	// This must come before ApplyVsync so the loaded vsyncMode is used.
	SaveData::current.LoadMachineSettings();

	// Adaptive sync (FreeSync / G-Sync) → standard vsync → uncapped
	ApplyVsync(SaveData::current.vsyncMode);

	/********************************   Default first scene   ***********************/
	BuildNewScene(SCENE_NUMBER::SCENETITLE);
	/********************************************************************************/
	return true;
}

/// This is the whole game
void SceneManager::Run() {
	timer->Start();
	isRunning = true;
	while (isRunning) {
		Uint64 frameStart = SDL_GetTicks();

		HandleEvents();
		timer->UpdateFrameTicks();

		// Start ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		currentScene->Update(timer->GetDeltaTime());
		currentScene->RenderBackground();
		currentScene->Render();
		currentScene->DrawGui();

		// Drain scene-switch request
		if (SceneSwitcher::hasPending) {
			GameScene gs = SceneSwitcher::pending;
			SceneSwitcher::hasPending = false;
			switch (gs) {
			case GameScene::TITLE: BuildNewScene(SCENE_NUMBER::SCENETITLE); break;
			case GameScene::MUN:   BuildNewScene(SCENE_NUMBER::SCENEMUN);   break;
			case GameScene::STG:   BuildNewScene(SCENE_NUMBER::SCENESTG);   break;
			case GameScene::JA:    BuildNewScene(SCENE_NUMBER::SCENEJA);    break;
			}
		}

		// Drain video settings request
		if (SceneSwitcher::hasVideoRequest) {
			SceneSwitcher::hasVideoRequest = false;
			// Exit fullscreen BEFORE resizing — SDL glitches if you resize
			// a window that is still in fullscreen mode.
			window->SetFullscreen(SceneSwitcher::videoFullscreen);
			if (!SceneSwitcher::videoFullscreen)
				window->SetSize(SceneSwitcher::videoWidth, SceneSwitcher::videoHeight);
			ApplyVsync(SceneSwitcher::videoVsync);
			// Write ONLY to settings.dat — never to a profile file.
			// Without this, the default profileName="Player" caused
			// profile_Player.dat to be created whenever video was changed
			// on the title screen before any profile was loaded.
			SaveData::current.SaveMachineSettings();
		}

		// Render ImGui
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		SDL_GL_SwapWindow(window->getWindow());

		// Frame pacing:
		// When vsync is active, SwapWindow already blocks for the display interval.
		// A manual sleep on top would halve the frame rate — so skip it.
		// When vsync is off, enforce a 60 fps floor manually.
		if (!vsyncActive) {
			const Uint64 kFrameMs = 16; // ~60 fps
			Uint64 elapsed = SDL_GetTicks() - frameStart;
			if (elapsed < kFrameMs)
				SDL_Delay(static_cast<Uint32>(kFrameMs - elapsed));
		}
	}
}

void SceneManager::ApplyVsync(int mode) {
	// Try the requested mode; if adaptive (-1) isn't supported, fall back.
	if (SDL_GL_SetSwapInterval(mode)) {
		vsyncActive = (mode != 0);
		SaveData::current.vsyncMode = mode;
	} else if (mode == -1 && SDL_GL_SetSwapInterval(1)) {
		vsyncActive = true;
		SaveData::current.vsyncMode = 1;
	} else {
		SDL_GL_SetSwapInterval(0);
		vsyncActive = false;
		SaveData::current.vsyncMode = 0;
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
	case SCENE_NUMBER::SCENETITLE:
		currentScene = new SceneTitle();
		status = currentScene->OnCreate();
		break;

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


