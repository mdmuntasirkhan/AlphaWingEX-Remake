#pragma once
#ifndef SCENE_H
#define SCENE_H

// ============================================================
//  Scene — abstract base class for all game screens.
//
//  Every concrete scene (SceneTitle, SceneMuntasir, etc.) must
//  implement the pure-virtual methods below. SceneManager calls
//  them in this order each frame:
//
//    OnCreate()          — one-time load: meshes, shaders, audio, save data
//    Update(dt)          — game logic, physics, input (not while paused)
//    RenderBackground()  — pre-3D pass: clear + nebula gradient (optional override)
//    Render()            — 3D draw pass
//    DrawGui()           — ImGui overlay (HUD, menus) (optional override)
//    OnDestroy()         — release every resource allocated in OnCreate()
//
//  HandleEvents() is called once per SDL event before Update().
//  OnVideoChanged() fires after a resolution or fullscreen change.
// ============================================================

union SDL_Event;

class Scene {
public:
	virtual ~Scene() {}

	virtual bool OnCreate()  = 0;
	virtual void OnDestroy() = 0;
	virtual void Update(const float deltaTime) = 0;
	virtual void Render() const = 0;
	virtual void HandleEvents(const SDL_Event& sdlEvent) = 0;

	virtual void RenderBackground() {}
	virtual void OnVideoChanged(int /*w*/, int /*h*/) {}
	virtual void DrawGui() {}
};

#endif // SCENE_H