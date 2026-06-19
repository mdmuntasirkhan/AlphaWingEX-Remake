#pragma once
#ifndef SCENETITLE_H
#define SCENETITLE_H

#include "Scene.h"
#include "SaveData.h"
#include "Sound.h"
#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <utility>

union SDL_Event;

class SceneTitle : public Scene {
private:
    enum class TitleState { MAIN, NEW_GAME_NAME, LOAD_SELECT };
    TitleState state;

    char                     nameBuf[32];
    std::vector<std::string> profiles;
    bool                     showSettings;
    int                      pendingDeleteIndex;

    // Leaderboard cache (sorted by high score descending)
    std::vector<std::pair<std::string, int>> leaderboard;

    // Audio
    // sfxStream   — full SFX gain for click/confirm sounds
    // hoverStream — same selectSound at ~18 % gain for hover feedback
    SDL_AudioStream* sfxStream;
    SDL_AudioStream* hoverStream;
    Sound*           selectSound;
    unsigned int     lastHoveredId;
    Uint64           lastHoverTick;

    void PlaySelect();
    void PlayHover();
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
