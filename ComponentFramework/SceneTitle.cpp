#include "SceneTitle.h"
#include "SceneSwitcher.h"
#include <glew.h>
#include <SDL3/SDL_events.h>
#include "imgui.h"
#include <cstring>

SceneTitle::SceneTitle()
    : state(TitleState::MAIN)
    , showSettings(false)
    , sfxStream(nullptr)
    , selectSound(nullptr)
{
    memset(nameBuf, 0, sizeof(nameBuf));
}

SceneTitle::~SceneTitle() {}

bool SceneTitle::OnCreate() {
    // Selection SFX
    SDL_AudioSpec spec;
    spec.freq     = 44100;
    spec.channels = 2;
    spec.format   = SDL_AUDIO_S16;
    sfxStream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (sfxStream) {
        SDL_SetAudioStreamGain(sfxStream, SaveData::current.sfxVolume);
        SDL_ResumeAudioStreamDevice(sfxStream);
    }

    selectSound = new Sound("audio/sfx/Select01.wav");
    selectSound->OnCreate();

    RefreshProfiles();
    return true;
}

void SceneTitle::OnDestroy() {
    if (selectSound) {
        selectSound->OnDestroy();
        delete selectSound;
        selectSound = nullptr;
    }
    if (sfxStream) {
        SDL_DestroyAudioStream(sfxStream);
        sfxStream = nullptr;
    }
}

void SceneTitle::Update(const float) {}
void SceneTitle::Render() const {}
void SceneTitle::HandleEvents(const SDL_Event&) {}

void SceneTitle::RenderBackground() {
    glClearColor(0.02f, 0.02f, 0.07f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void SceneTitle::PlaySelect() {
    if (selectSound && sfxStream) selectSound->Play(sfxStream);
}

void SceneTitle::RefreshProfiles() {
    profiles = SaveData::GetProfileList();
}

void SceneTitle::DrawGui() {
    ImGuiIO& io  = ImGui::GetIO();
    float    cx  = io.DisplaySize.x * 0.5f;
    float    cy  = io.DisplaySize.y * 0.5f;

    // ── title banner ───────────────────────────────────────────────────────
    ImGui::SetNextWindowPos(ImVec2(cx, cy * 0.32f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 90), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::Begin("##banner", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoInputs);

    float iw = ImGui::GetContentRegionAvail().x;
    ImGui::SetWindowFontScale(2.4f);
    ImVec2 tSz = ImGui::CalcTextSize("ALPHA WING EX");
    ImGui::SetCursorPosX((iw - tSz.x) * 0.5f);
    ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "ALPHA WING EX");
    ImGui::SetWindowFontScale(0.85f);
    ImVec2 sSz = ImGui::CalcTextSize("R  E  M  A  K  E");
    ImGui::SetCursorPosX((iw - sSz.x) * 0.5f);
    ImGui::TextColored(ImVec4(0.40f, 0.40f, 0.60f, 1.0f), "R  E  M  A  K  E");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::End();

    // ── main panel ─────────────────────────────────────────────────────────
    float panH = 0.0f;
    if      (state == TitleState::MAIN)          panH = 200.0f + (showSettings ? 130.0f : 0.0f);
    else if (state == TitleState::NEW_GAME_NAME) panH = 160.0f;
    else if (state == TitleState::LOAD_SELECT)   panH = 60.0f + (float)profiles.size() * 46.0f + 50.0f;

    float panW = 420.0f;
    ImGui::SetNextWindowPos(ImVec2(cx, cy * 1.12f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(panW, panH), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.88f);
    ImGui::Begin("##menu", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

    float innerW = ImGui::GetContentRegionAvail().x;
    float btnW   = innerW - 16.0f;
    float btnX   = 8.0f;

    // ── MAIN ──────────────────────────────────────────────────────────────
    if (state == TitleState::MAIN) {
        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button("NEW GAME", ImVec2(btnW, 44))) {
            PlaySelect();
            memset(nameBuf, 0, sizeof(nameBuf));
            state = TitleState::NEW_GAME_NAME;
        }

        ImGui::Spacing();
        ImGui::SetCursorPosX(btnX);
        bool hasProfiles = !profiles.empty();
        if (!hasProfiles) ImGui::BeginDisabled();
        if (ImGui::Button("LOAD GAME", ImVec2(btnW, 44))) {
            PlaySelect();
            RefreshProfiles();
            state = TitleState::LOAD_SELECT;
        }
        if (!hasProfiles) ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button(showSettings ? "SETTINGS  [hide]" : "SETTINGS  [show]",
                          ImVec2(btnW, 38))) {
            PlaySelect();
            showSettings = !showSettings;
        }

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
            if (ImGui::SliderFloat("##sv", &SaveData::current.sfxVolume, 0.0f, 1.0f)) {
                if (sfxStream)
                    SDL_SetAudioStreamGain(sfxStream, SaveData::current.sfxVolume);
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button("EXIT", ImVec2(btnW, 32))) {
            PlaySelect();
            SDL_Event e; e.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&e);
        }
    }

    // ── NEW GAME — enter name ─────────────────────────────────────────────
    else if (state == TitleState::NEW_GAME_NAME) {
        ImGui::Spacing();
        ImGui::SetCursorPosX(btnX);
        ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "Enter pilot name:");
        ImGui::Spacing();
        ImGui::SetCursorPosX(btnX);
        ImGui::SetNextItemWidth(btnW);
        ImGui::InputText("##pname", nameBuf, sizeof(nameBuf));

        ImGui::Spacing();
        ImGui::SetCursorPosX(btnX);
        bool nameOk = nameBuf[0] != '\0';
        if (!nameOk) ImGui::BeginDisabled();
        if (ImGui::Button("START MISSION", ImVec2(btnW * 0.55f, 42))) {
            PlaySelect();
            SaveData::current.Reset();
            SaveData::current.profileName = nameBuf;
            SaveData::current.Save();
            SceneSwitcher::Request(GameScene::MUN);
        }
        if (!nameOk) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("BACK", ImVec2(-1, 42))) {
            PlaySelect();
            state = TitleState::MAIN;
        }
    }

    // ── LOAD — pick profile ───────────────────────────────────────────────
    else if (state == TitleState::LOAD_SELECT) {
        ImGui::SetCursorPosX(btnX);
        ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "Select pilot:");
        ImGui::Spacing();

        for (auto& pname : profiles) {
            ImGui::SetCursorPosX(btnX);
            if (ImGui::Button(pname.c_str(), ImVec2(btnW, 40))) {
                PlaySelect();
                SaveData::current.Load(pname);
                SceneSwitcher::Request(GameScene::MUN);
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button("BACK", ImVec2(btnW, 34))) {
            PlaySelect();
            state = TitleState::MAIN;
        }
    }

    ImGui::End();
}
