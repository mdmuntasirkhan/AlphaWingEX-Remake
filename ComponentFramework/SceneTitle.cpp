#include "SceneTitle.h"
#include "SceneSwitcher.h"
#include <glew.h>
#include <SDL3/SDL_events.h>
#include "imgui.h"
#include <cstring>

SceneTitle::SceneTitle()
    : state(TitleState::MAIN)
    , showSettings(false)
    , pendingDeleteIndex(-1)
    , sfxStream(nullptr)
    , hoverStream(nullptr)
    , selectSound(nullptr)
    , lastHoveredId(0)
    , lastHoverTick(0)
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

    hoverStream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (hoverStream) {
        SDL_SetAudioStreamGain(hoverStream, SaveData::current.sfxVolume * 0.35f);
        SDL_ResumeAudioStreamDevice(hoverStream);
    }

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
    if (hoverStream) {
        SDL_DestroyAudioStream(hoverStream);
        hoverStream = nullptr;
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

void SceneTitle::PlayHover() {
    if (!selectSound || !hoverStream) return;
    unsigned int id  = ImGui::GetItemID();
    Uint64       now = SDL_GetTicks();
    if (id == lastHoveredId || now - lastHoverTick < 150) return;
    lastHoveredId = id;
    lastHoverTick = now;
    selectSound->Play(hoverStream);
}

void SceneTitle::RefreshProfiles() {
    profiles    = SaveData::GetProfileList();
    leaderboard = SaveData::GetLeaderboard();
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

    // ── leaderboard panel (right side, always visible) ─────────────────────
    {
        float lbW = 290.0f;
        float lbH = 76.0f + (float)(leaderboard.empty() ? 1 : (int)leaderboard.size()) * 21.0f;
        ImGui::SetNextWindowPos(
            ImVec2(io.DisplaySize.x - 20.0f, io.DisplaySize.y * 0.24f),
            ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(lbW, lbH), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.82f);
        ImGui::Begin("##leaderboard", nullptr,
            ImGuiWindowFlags_NoResize  | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoInputs);

        float lbIW = ImGui::GetContentRegionAvail().x;
        ImGui::SetWindowFontScale(1.25f);
        ImVec2 hSz = ImGui::CalcTextSize("HIGH SCORES");
        ImGui::SetCursorPosX((lbIW - hSz.x) * 0.5f);
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.1f, 1.0f), "HIGH SCORES");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), " %-3s %-14s %s", "#", "PILOT", "SCORE");
        ImGui::Separator();

        if (leaderboard.empty()) {
            ImGui::TextDisabled("  No profiles yet.");
        } else {
            static const ImVec4 kRankCol[3] = {
                ImVec4(1.0f,  0.84f, 0.0f,  1.0f), // gold
                ImVec4(0.75f, 0.75f, 0.75f, 1.0f), // silver
                ImVec4(0.80f, 0.50f, 0.20f, 1.0f), // bronze
            };
            for (int i = 0; i < (int)leaderboard.size(); i++) {
                ImVec4 col = (i < 3) ? kRankCol[i] : ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
                if (leaderboard[i].second > 0)
                    ImGui::TextColored(col, " %-3d %-14s %d",
                        i + 1, leaderboard[i].first.c_str(), leaderboard[i].second);
                else
                    ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f),
                        " %-3d %-14s --", i + 1, leaderboard[i].first.c_str());
            }
        }
        ImGui::End();
    }

    // ── main panel ─────────────────────────────────────────────────────────
    bool atCap = (int)profiles.size() >= SaveData::kMaxProfiles;
    float panH = 0.0f;
    if      (state == TitleState::MAIN)          panH = 200.0f + (showSettings ? 130.0f : 0.0f) + (atCap ? 46.0f : 0.0f);
    else if (state == TitleState::NEW_GAME_NAME) panH = 160.0f;
    else if (state == TitleState::LOAD_SELECT)   panH = 60.0f + (float)profiles.size() * 52.0f + 50.0f;

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
        if (atCap) ImGui::BeginDisabled();
        if (ImGui::Button("NEW GAME", ImVec2(btnW, 44))) {
            PlaySelect();
            memset(nameBuf, 0, sizeof(nameBuf));
            state = TitleState::NEW_GAME_NAME;
        }
        if (ImGui::IsItemHovered()) PlayHover();
        if (atCap) {
            ImGui::EndDisabled();
            ImGui::SetCursorPosX(btnX);
            ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.15f, 1.0f),
                "Profile limit reached (%d/%d).", (int)profiles.size(), SaveData::kMaxProfiles);
            ImGui::SetCursorPosX(btnX);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                "Delete a slot in LOAD GAME to free space.");
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
        if (ImGui::IsItemHovered()) PlayHover();
        if (!hasProfiles) ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button(showSettings ? "SETTINGS  [hide]" : "SETTINGS  [show]",
                          ImVec2(btnW, 38))) {
            PlaySelect();
            showSettings = !showSettings;
        }
        if (ImGui::IsItemHovered()) PlayHover();

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
                    SDL_SetAudioStreamGain(sfxStream,   SaveData::current.sfxVolume);
                if (hoverStream)
                    SDL_SetAudioStreamGain(hoverStream, SaveData::current.sfxVolume * 0.35f);
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
        if (ImGui::IsItemHovered()) PlayHover();
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
        if (ImGui::IsItemHovered()) PlayHover();
        if (!nameOk) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("BACK", ImVec2(-1, 42))) {
            PlaySelect();
            state = TitleState::MAIN;
        }
        if (ImGui::IsItemHovered()) PlayHover();
    }

    // ── LOAD — pick profile ───────────────────────────────────────────────
    else if (state == TitleState::LOAD_SELECT) {
        ImGui::SetCursorPosX(btnX);
        ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "Select pilot:");
        ImGui::Spacing();

        float delBtnW  = 38.0f;
        float spacing  = ImGui::GetStyle().ItemSpacing.x;
        float loadBtnW = btnW - delBtnW - spacing;

        for (int i = 0; i < (int)profiles.size(); i++) {
            std::string pname = profiles[i];
            ImGui::SetCursorPosX(btnX);

            if (pendingDeleteIndex == i) {
                bool doDelete = false;
                bool doCancel = false;
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                    "Delete \"%s\"?", pname.c_str());
                ImGui::SameLine();
                float confirmX = btnX + btnW - 172.0f;
                ImGui::SetCursorPosX(confirmX);
                if (ImGui::Button(("Yes##" + pname).c_str(), ImVec2(80.0f, 0.0f))) doDelete = true;
                if (ImGui::IsItemHovered()) PlayHover();
                ImGui::SameLine();
                if (ImGui::Button(("No##"  + pname).c_str(), ImVec2(80.0f, 0.0f))) doCancel = true;
                if (ImGui::IsItemHovered()) PlayHover();

                if (doDelete) {
                    SaveData::DeleteProfile(pname);
                    RefreshProfiles();
                    pendingDeleteIndex = -1;
                    break;
                }
                if (doCancel) pendingDeleteIndex = -1;
            } else {
                if (ImGui::Button(pname.c_str(), ImVec2(loadBtnW, 40.0f))) {
                    PlaySelect();
                    SaveData::current.Load(pname);
                    SceneSwitcher::Request(GameScene::MUN);
                }
                if (ImGui::IsItemHovered()) PlayHover();
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f, 0.1f, 0.1f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f,  0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.0f,  0.3f, 0.3f, 1.0f));
                if (ImGui::Button(("X##" + pname).c_str(), ImVec2(delBtnW, 40.0f))) {
                    PlaySelect();
                    pendingDeleteIndex = i;
                }
                if (ImGui::IsItemHovered()) PlayHover();
                ImGui::PopStyleColor(3);
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button("BACK", ImVec2(btnW, 34))) {
            PlaySelect();
            pendingDeleteIndex = -1;
            state = TitleState::MAIN;
        }
        if (ImGui::IsItemHovered()) PlayHover();
    }

    ImGui::End();
}
