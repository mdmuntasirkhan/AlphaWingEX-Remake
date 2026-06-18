#include "SceneTitle.h"
#include "SceneSwitcher.h"
#include <glew.h>
#include <SDL3/SDL_events.h>
#include "imgui.h"

SceneTitle::SceneTitle() : showSettings(false), saveExists(false) {}
SceneTitle::~SceneTitle() {}

bool SceneTitle::OnCreate() {
    saveExists = SaveData::current.Load();
    if (!saveExists) SaveData::current.Reset();
    return true;
}

void SceneTitle::OnDestroy() {}
void SceneTitle::Update(const float) {}
void SceneTitle::Render() const {}
void SceneTitle::HandleEvents(const SDL_Event&) {}

void SceneTitle::RenderBackground() {
    glClearColor(0.02f, 0.02f, 0.07f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void SceneTitle::DrawGui() {
    ImGuiIO& io  = ImGui::GetIO();
    float    cx  = io.DisplaySize.x * 0.5f;
    float    cy  = io.DisplaySize.y * 0.5f;
    float    panW = 440.0f;
    float    panH = 310.0f + (showSettings ? 135.0f : 0.0f)
                            + (saveExists   ?  30.0f : 0.0f);

    ImGui::SetNextWindowPos(ImVec2(cx, cy), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(panW, panH), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.90f);
    ImGui::Begin("##title", nullptr,
        ImGuiWindowFlags_NoResize    |
        ImGuiWindowFlags_NoMove      |
        ImGuiWindowFlags_NoTitleBar  |
        ImGuiWindowFlags_NoScrollbar);

    float innerW = ImGui::GetContentRegionAvail().x;
    float btnW   = innerW - 20.0f;
    float btnX   = 10.0f;

    // --- Title ---
    ImGui::SetWindowFontScale(2.2f);
    ImVec2 tSz = ImGui::CalcTextSize("ALPHA WING EX");
    ImGui::SetCursorPosX((innerW - tSz.x) * 0.5f);
    ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "ALPHA WING EX");

    ImGui::SetWindowFontScale(0.85f);
    ImVec2 sSz = ImGui::CalcTextSize("R  E  M  A  K  E");
    ImGui::SetCursorPosX((innerW - sSz.x) * 0.5f);
    ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.65f, 1.0f), "R  E  M  A  K  E");
    ImGui::SetWindowFontScale(1.0f);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // --- Saved progress summary ---
    if (saveExists) {
        ImGui::SetCursorPosX(btnX);
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.15f, 1.0f),
            "Shards: %d    High Score: %d",
            SaveData::current.shardCount,
            SaveData::current.highScore);
        ImGui::Spacing();
    }

    // --- Buttons ---
    ImGui::SetCursorPosX(btnX);
    if (ImGui::Button("NEW GAME", ImVec2(btnW, 44))) {
        SaveData::current.Reset();
        SceneSwitcher::Request(GameScene::MUN);
    }

    ImGui::Spacing();
    ImGui::SetCursorPosX(btnX);
    if (!saveExists) ImGui::BeginDisabled();
    if (ImGui::Button("LOAD GAME", ImVec2(btnW, 44))) {
        // SaveData::current already loaded in OnCreate
        SceneSwitcher::Request(GameScene::MUN);
    }
    if (!saveExists) ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::SetCursorPosX(btnX);
    if (ImGui::Button(showSettings ? "SETTINGS  [hide]" : "SETTINGS  [show]",
                      ImVec2(btnW, 38))) {
        showSettings = !showSettings;
    }

    // --- Settings panel ---
    if (showSettings) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetCursorPosX(btnX);
        ImGui::Text("Music Volume");
        ImGui::SetCursorPosX(btnX);
        ImGui::SetNextItemWidth(btnW);
        ImGui::SliderFloat("##mv", &SaveData::current.musicVolume, 0.0f, 1.0f);

        ImGui::SetCursorPosX(btnX);
        ImGui::Text("SFX Volume");
        ImGui::SetCursorPosX(btnX);
        ImGui::SetNextItemWidth(btnW);
        ImGui::SliderFloat("##sv", &SaveData::current.sfxVolume, 0.0f, 1.0f);

        ImGui::Spacing();
        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button("Save Settings", ImVec2(btnW, 32))) {
            SaveData::current.Save();
            saveExists = true;
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SetCursorPosX(btnX);
    if (ImGui::Button("EXIT", ImVec2(btnW, 32))) {
        SDL_Event e;
        e.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&e);
    }

    ImGui::End();
}
