#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include <string>

class SceneManager {
private:
    // Internal scene IDs — public scene requests use GameScene in SceneSwitcher.h
    enum class SceneID {
        TITLE = 0,
        JA,
        MUN
    };

    class Scene* currentScene;
    class Timer* timer;
    class Window* window;

    bool isRunning;
    bool vsyncActive;  // true when swap interval != 0; skips the manual SDL_Delay cap

    bool BuildNewScene(SceneID id);
    void ApplyVsync(int mode);  // tries requested mode, falls back gracefully if unsupported

public:
    SceneManager();
    ~SceneManager();

    bool Initialize(std::string name, int width, int height);
    void Run();
    void HandleEvents();
};

#endif // SCENEMANAGER_H
