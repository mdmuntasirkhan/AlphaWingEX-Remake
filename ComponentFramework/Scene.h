#pragma once
#ifndef SCENE_H
#define SCENE_H

union SDL_Event;

class Scene {
public:
	virtual ~Scene() {}

	virtual bool OnCreate()  = 0;
	virtual void OnDestroy() = 0;
	virtual void Update(const float deltaTime) = 0;
	virtual void Render() const = 0;
	virtual void HandleEvents(const SDL_Event& sdlEvent) = 0;

	// Optional overrides — default implementations do nothing
	virtual void RenderBackground() {}
	virtual void OnVideoChanged(int /*w*/, int /*h*/) {}
	virtual void DrawGui() {}
};

#endif // SCENE_H