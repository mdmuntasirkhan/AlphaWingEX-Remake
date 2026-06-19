#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include <string>
class SceneManager  {
public:
	
	SceneManager();
	~SceneManager();
	void Run();
	bool Initialize(std::string name_, int width_, int height_);
	void HandleEvents();
	
	
private:
	enum class SCENE_NUMBER {
		SCENETITLE = 0,
		SCENEJA,
		SCENEMUN,
		SCENESTG
	};

	class Scene* currentScene;
	class Timer* timer;
	class Window* window;

	unsigned int fps;
	bool isRunning;
	bool fullScreen;
	bool vsyncActive;   // true when swap interval != 0; skips manual SDL_Delay
	bool BuildNewScene(SCENE_NUMBER scene_);
	void ApplyVsync(int mode); // tries mode, falls back gracefully
};


#endif // SCENEMANAGER_H