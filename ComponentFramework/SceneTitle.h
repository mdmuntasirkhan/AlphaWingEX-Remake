#pragma once
#ifndef SCENETITLE_H
#define SCENETITLE_H

#include "Scene.h"
#include "SaveData.h"
#include "Sound.h"
#include <SDL3/SDL.h>
#include <string>
#include <vector>

union SDL_Event;

class SceneTitle : public Scene {
private:
    enum class TitleState { MAIN, NEW_GAME_NAME, LOAD_SELECT };
    TitleState state;

    char        nameBuf[32];
    std::vector<std::string> profiles;
    bool        showSettings;

    // Selection sound
    SDL_AudioStream* sfxStream;
    Sound*           selectSound;

    void PlaySelect();
    void RefreshProfiles();

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
