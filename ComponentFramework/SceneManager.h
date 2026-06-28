#pragma once
#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

// ============================================================
//  SceneManager — owns the game loop and scene lifecycle.
//
//  Call Initialize() once at startup, then Run() blocks until quit.
//
//  Each frame:
//    HandleEvents() → scene Update/RenderBackground/Render/DrawGui
//    → drain SceneSwitcher (deferred scene change)
//    → drain video request (deferred resolution/vsync change)
//    → ImGui::Render() → SDL_GL_SwapWindow
//
//  Scene switches are deferred until after DrawGui() so the current
//  frame completes cleanly before BuildNewScene() destroys the old scene.
// ============================================================

#include <string>

class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    bool Initialize(std::string name, int width, int height);
    void Run();
    void HandleEvents();

private:
    // Internal scene identifiers — used only by BuildNewScene().
    // The public API uses GameScene (SceneSwitcher.h) for requesting switches.
    enum class SceneID {
        TITLE = 0,
        JA,
        MUN
    };

    class Scene*  currentScene;
    class Timer*  timer;
    class Window* window;

    bool isRunning;
    bool vsyncActive;  // true when swap interval != 0; skips the manual SDL_Delay cap

    bool BuildNewScene(SceneID id);
    void ApplyVsync(int mode);  // tries requested mode, falls back gracefully if unsupported
};

#endif // SCENEMANAGER_H