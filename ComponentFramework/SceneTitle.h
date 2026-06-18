#pragma once
#ifndef SCENETITLE_H
#define SCENETITLE_H

#include "Scene.h"
#include "SaveData.h"

union SDL_Event;

class SceneTitle : public Scene {
private:
    bool showSettings;
    bool saveExists;

public:
    SceneTitle();
    virtual ~SceneTitle();

    virtual bool OnCreate() override;
    virtual void OnDestroy() override;
    virtual void Update(const float deltaTime) override;
    virtual void RenderBackground() override;
    virtual void Render() const override;
    virtual void HandleEvents(const SDL_Event& sdlEvent) override;
    virtual void DrawGui() override;
};

#endif
