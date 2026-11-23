// AlphaWingEX-Remake
// Brief: HUD, pause menu, and game-over screen drawing — extracted from SceneMuntasir.

#include <cmath>
#include <SDL.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_events.h>
#include "HUDRenderer.h"
#include "imgui.h"
#include "SaveData.h"
#include "SceneSwitcher.h"
#include "GameConstants.h"
#include "Version.h"
#include "Fonts.h"
#include "Player.h"
#include "Bullet.h"
#include "Environment.h"
#include "ShardBeacon.h"
#include "LevelDirector.h"
#include "Asteroid.h"
#include "Bot01.h"
#include "Bot02.h"
#include "Sound.h"

HUDRenderer::HUDRenderer() :
    ctx{},
    pauseShowSettings{ false },
    pendingResIndex{ SaveData::current.resolutionIndex },
    pendingFullscreen{ SaveData::current.fullscreen },
    pendingVsync{ SaveData::current.vsyncMode },
    pendingTargetFPS{ SaveData::current.targetFPS },
    musicPaused{ false },
    lastHoveredId{ 0 },
    lastHoverTick{ 0 } {
}

HUDRenderer::~HUDRenderer() {
}

void HUDRenderer::Init(const Context& context) {
    ctx = context;
}

// Plays a quiet click when the cursor first moves onto a new ImGui widget.
void HUDRenderer::PlayHoverSound() {
    if (!ctx.uiClickSound || !ctx.hoverStream || !ImGui::IsItemHovered()) return;
    unsigned int id  = ImGui::GetItemID();
    Uint64       now = SDL_GetTicks();
    if (id == lastHoveredId || now - lastHoverTick < 150) return;
    lastHoveredId = id;
    lastHoverTick = now;
    ctx.uiClickSound->Play(ctx.hoverStream);
}

// HUD
void HUDRenderer::DrawHUD() {

    // Game HUD (always visible)
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(318, 292), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.75f);
    ImGui::Begin("##hud", nullptr,
        ImGuiWindowFlags_NoResize   | ImGuiWindowFlags_NoMove        |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar   |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f),
        "PILOT: %s", SaveData::current.profileName.c_str());
    ImGui::Text("SCORE: %-8d   HI: %d", *ctx.score, SaveData::current.highScore);
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.1f, 1.0f), "SHARDS: %d", *ctx.shardCount);
    // Missile slots — solid colored bars (green=ready, dark=spent) + vertical reload bar
    ImGui::Text("MISSILES");
    const ImVec2 slotSize(14.0f, 22.0f);
    const float  slotGap  = 3.0f;
    int maxM = ctx.bullet->GetMaxMissiles();
    int curM = ctx.bullet->GetMissileCount();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (int mi = 0; mi < maxM; mi++) {
        if (mi > 0) ImGui::SameLine(0.0f, slotGap);
        ImVec2 p = ImGui::GetCursorScreenPos();
        bool  ready = (mi < curM);
        ImU32 fill  = ready ? IM_COL32(55, 210, 80, 255) : IM_COL32(28, 28, 28, 220);
        dl->AddRectFilled(p, ImVec2(p.x + slotSize.x, p.y + slotSize.y), fill, 2.0f);
        dl->AddRect(p, ImVec2(p.x + slotSize.x, p.y + slotSize.y), IM_COL32(160, 160, 160, 200), 2.0f);
        ImGui::Dummy(slotSize);
    }
    // Vertical reload bar — fills from the bottom up while a slot reloads
    if (curM < maxM) {
        ImGui::SameLine(0.0f, slotGap * 2.0f);
        float   frac    = ctx.bullet->GetReloadFraction();
        ImVec2  p       = ImGui::GetCursorScreenPos();
        ImVec2  barSize(7.0f, slotSize.y);
        float   fillH   = barSize.y * frac;
        dl->AddRectFilled(p, ImVec2(p.x + barSize.x, p.y + barSize.y), IM_COL32(28, 28, 28, 220), 2.0f);
        dl->AddRectFilled(ImVec2(p.x, p.y + barSize.y - fillH),
                          ImVec2(p.x + barSize.x, p.y + barSize.y),
                          IM_COL32(255, 165, 30, 230), 2.0f);
        dl->AddRect(p, ImVec2(p.x + barSize.x, p.y + barSize.y), IM_COL32(160, 160, 160, 200), 2.0f);
        ImGui::Dummy(barSize);
    }

    // Lives
    ImGui::Text("LIVES:");
    for (int i = 0; i < ctx.player->GetLives(); i++) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "[*]");
    }

    // Health bar — green / yellow / red
    float hp = ctx.player->GetHealth() / 100.0f;
    ImVec4 hpCol = hp > 0.6f ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f)
                 : hp > 0.3f ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f)
                             : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, hpCol);
    ImGui::ProgressBar(hp, ImVec2(-1.0f, 20.0f), "");
    ImGui::PopStyleColor();

    // Three-chunk shield bar matching the three recharge penalty tiers
    {
        float charge = ctx.player->GetShieldChargeFraction();

        if (ctx.player->IsShieldActive()) {
            ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "SHIELD ACTIVE  [E]");
        } else if (ctx.player->IsShieldRecharging()) {
            ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.75f, 1.0f), "SHIELD RECHARGING");
        } else {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "SHIELD READY  [E]");
        }

        ImDrawList* dl   = ImGui::GetWindowDrawList();
        ImVec2      pos  = ImGui::GetCursorScreenPos();
        float       barW = ImGui::GetContentRegionAvail().x;
        const float barH = 12.0f;
        const float gap  = 3.0f;

        // Chunk widths — proportional to their charge range
        float wRed  = barW * 0.10f;
        float wOrg  = barW * 0.10f;
        float wCyan = barW * 0.80f - gap * 2.0f;

        float xRed  = pos.x;
        float xOrg  = xRed  + wRed  + gap;
        float xCyan = xOrg  + wOrg  + gap;
        float y0    = pos.y;
        float y1    = pos.y + barH;

        // Fill fractions — each chunk fills independently for its tier
        float fRed  = charge <= 0.10f ? charge / 0.10f : 1.0f;
        float fOrg  = charge <= 0.10f ? 0.0f : charge <= 0.20f ? (charge - 0.10f) / 0.10f : 1.0f;
        float fCyan = charge <= 0.20f ? 0.0f : (charge - 0.20f) / 0.80f;

        const ImU32 bgCol   = IM_COL32( 25,  25,  45, 210);
        const ImU32 redCol  = IM_COL32(220,  35,  35, 255);
        const ImU32 orgCol  = IM_COL32(255, 140,   0, 255);
        const ImU32 cyanCol = IM_COL32(  0, 185, 255, 255);
        const ImU32 rimCol  = IM_COL32(255, 255, 255,  35);
        const float r       = 2.0f;

        // Red chunk
        dl->AddRectFilled(ImVec2(xRed,  y0), ImVec2(xRed  + wRed,  y1), bgCol,  r);
        if (fRed  > 0.0f) dl->AddRectFilled(ImVec2(xRed,  y0), ImVec2(xRed  + wRed  * fRed,  y1), redCol,  r);
        dl->AddRect(      ImVec2(xRed,  y0), ImVec2(xRed  + wRed,  y1), rimCol, r);

        // Orange chunk
        dl->AddRectFilled(ImVec2(xOrg,  y0), ImVec2(xOrg  + wOrg,  y1), bgCol,  r);
        if (fOrg  > 0.0f) dl->AddRectFilled(ImVec2(xOrg,  y0), ImVec2(xOrg  + wOrg  * fOrg,  y1), orgCol,  r);
        dl->AddRect(      ImVec2(xOrg,  y0), ImVec2(xOrg  + wOrg,  y1), rimCol, r);

        // Cyan chunk
        dl->AddRectFilled(ImVec2(xCyan, y0), ImVec2(xCyan + wCyan, y1), bgCol,  r);
        if (fCyan > 0.0f) dl->AddRectFilled(ImVec2(xCyan, y0), ImVec2(xCyan + wCyan * fCyan, y1), cyanCol, r);
        dl->AddRect(      ImVec2(xCyan, y0), ImVec2(xCyan + wCyan, y1), rimCol, r);

        ImGui::Dummy(ImVec2(barW, barH));
    }

    // Lost shard beacon status
    if (*ctx.beaconTriggerTime > 0.0f && !ctx.shardBeacon->IsActive()) {
        float timeLeft = *ctx.beaconTriggerTime - ctx.levelDirector->GetTime();
        ImGui::TextColored(ImVec4(1.0f, 0.70f, 0.10f, 0.85f),
            "BEACON IN %.0fs  [%d shards]",
            timeLeft, SaveData::current.lostShardCount);
    } else if (ctx.shardBeacon->IsActive()) {
        ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.05f, 1.0f),
            ">> RECOVER %d LOST SHARDS", ctx.shardBeacon->GetCount());
    }

    // Warp Drive charge — always visible in HUD
    ImGui::Separator();
    {
        bool  charging = *ctx.Q_Held && !ctx.environment->IsWarpActive();
        float progress = charging ? (*ctx.Q_HoldTimer / 3.0f) : 0.0f;
        float t        = (float)ImGui::GetTime();

        // Label left, hint right-aligned to avoid border clipping
        ImVec4 labelCol = charging
            ? ImVec4(0.4f + progress * 0.6f, 0.7f + progress * 0.3f, 1.0f, 1.0f)
            : ImVec4(0.55f, 0.80f, 1.0f, 0.95f);
        ImGui::TextColored(labelCol, "WARP DRIVE");
        const char* hint = charging ? "release to cancel" : "HOLD Q";
        float hintW = ImGui::CalcTextSize(hint).x;
        ImGui::SameLine(0, 0);
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - hintW - ImGui::GetStyle().WindowPadding.x);
        ImGui::TextDisabled("%s", hint);

        // Full-width bar
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.04f, 0.04f, 0.12f, 0.95f));
        if (charging) {
            // Charging: dark-blue → bright cyan fill
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                ImVec4(progress * 0.15f, 0.35f + progress * 0.65f, 1.0f, 1.0f));
            ImGui::ProgressBar(progress, ImVec2(-1.0f, 16.0f), "");
            ImGui::PopStyleColor(2);
        } else {
            // Idle: full bar with animated electric-blue → violet gradient
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.05f, 0.08f, 0.18f, 1.0f));
            ImGui::ProgressBar(1.0f, ImVec2(-1.0f, 16.0f), "");
            ImGui::PopStyleColor(2);
            float  pulse = 0.70f + 0.30f * sinf(t * 1.4f);
            int    a     = (int)(248 * pulse);
            ImVec2 bMin  = ImGui::GetItemRectMin();
            ImVec2 bMax  = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddRectFilledMultiColor(
                bMin, bMax,
                IM_COL32(0,   195, 255, a),   // electric blue (left)
                IM_COL32(160,  35, 255, a),   // violet (right)
                IM_COL32(160,  35, 255, a),
                IM_COL32(0,   195, 255, a)
            );
        }
    }

    ImGui::TextDisabled("ESC  Pause");
    ImGui::End();

    // Level Timer
    {
        float levelSec = ctx.levelDirector->GetTime();
        int   mm       = (int)(levelSec / 60.0f);
        int   ss       = (int)(levelSec) % 60;
        float t        = (float)ImGui::GetTime();

        ImGuiIO& io = ImGui::GetIO();
        // Camera debug sits at cx ± 190 (380 px wide, centered).
        // Pin the timer's right edge 10 px left of that panel's left edge.
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f - 200.0f, 20.0f),
                                ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.03f, 0.05f, 0.13f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_Border,   ImVec4(0.15f, 0.55f, 1.0f,  0.55f));
        ImGui::Begin("##leveltimer", nullptr,
            ImGuiWindowFlags_NoTitleBar     | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoMove         | ImGuiWindowFlags_NoScrollbar      |
            ImGuiWindowFlags_NoSavedSettings| ImGuiWindowFlags_NoNav            |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        // Small spaced header — body font (no scaling)
        ImGui::TextDisabled("  L E V E L  ");

        // Large MM:SS — dedicated 32 px raster, crisp at any resolution
        ImGui::PushFont(Fonts::large);
        float colonAlpha = 0.35f + 0.65f * sinf(t * GameConst::kPi);
        ImGui::TextColored(ImVec4(0.15f, 0.88f, 1.0f,  1.0f), "%02d", mm);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(ImVec4(1.0f,  1.0f,  1.0f,  colonAlpha), ":");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(ImVec4(0.65f, 0.25f, 1.0f,  1.0f), "%02d", ss);
        ImGui::PopFont();

        ImGui::End();
        ImGui::PopStyleColor(2);
    }

    // Build Version
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 10.0f, io.DisplaySize.y - 10.0f),
                                ImGuiCond_Always, ImVec2(1.0f, 1.0f));
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::Begin("##buildver", nullptr,
            ImGuiWindowFlags_NoTitleBar     | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoMove         | ImGuiWindowFlags_NoScrollbar      |
            ImGuiWindowFlags_NoSavedSettings| ImGuiWindowFlags_NoNav            |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoInputs);

        ImGui::TextDisabled("Alpha Engine  v%d.%d.%d  build %d",
            AppVersion::kMajor, AppVersion::kMinor, AppVersion::kPatch, AppVersion::kBuild);

        ImGui::End();
    }
}

// Pause Menu
void HUDRenderer::DrawPauseMenu() {
    if (!*ctx.gamePaused) return;

    ImGuiIO& io   = ImGui::GetIO();
        float    panW = 380.0f;
        float    panH = pauseShowSettings ? 640.0f : 275.0f;
        ImGui::SetNextWindowPos(
            ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(panW, panH), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.92f);
        ImGui::Begin("##pause", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

        float innerW = ImGui::GetContentRegionAvail().x;
        float btnW   = innerW - 16.0f;
        float btnX   = 8.0f;

        // "PAUSED" — large centred title, same style as title screen
        ImGui::SetWindowFontScale(2.0f);
        ImVec2 tSz = ImGui::CalcTextSize("PAUSED");
        ImGui::SetCursorPosX((innerW - tSz.x) * 0.5f);
        ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "PAUSED");
        ImGui::SetWindowFontScale(1.0f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button("RESUME", ImVec2(btnW, 40.0f)))
            *ctx.gamePaused = false;
        PlayHoverSound();

        ImGui::Spacing();

        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button(
                pauseShowSettings ? "SETTINGS  [hide]" : "SETTINGS  [show]",
                ImVec2(btnW, 32.0f))) {
            if (!pauseShowSettings) {
                pendingResIndex   = SaveData::current.resolutionIndex;
                pendingFullscreen = SaveData::current.fullscreen;
                pendingVsync      = SaveData::current.vsyncMode;
                pendingTargetFPS  = SaveData::current.targetFPS;
            }
            pauseShowSettings = !pauseShowSettings;
        }
        PlayHoverSound();

        if (pauseShowSettings) {
            // Audio
            ImGui::Spacing();
            ImGui::SetCursorPosX(btnX);
            ImGui::Text("Music Volume");
            ImGui::SetCursorPosX(btnX);
            ImGui::SetNextItemWidth(btnW);
            if (ImGui::SliderFloat("##music", ctx.musicVolume, 0.0f, 1.0f))
                SDL_SetAudioStreamGain(ctx.bgmPlayer, *ctx.musicVolume);

            ImGui::SetCursorPosX(btnX);
            ImGui::Text("SFX Volume");
            ImGui::SetCursorPosX(btnX);
            ImGui::SetNextItemWidth(btnW);
            if (ImGui::SliderFloat("##sfx", ctx.sfxVolume, 0.0f, 1.0f)) {
                SDL_SetAudioStreamGain(ctx.sfxPlayer,         *ctx.sfxVolume);
                SDL_SetAudioStreamGain(ctx.sfxLaserHitStream, *ctx.sfxVolume * 2.0f);
                if (ctx.hoverStream) SDL_SetAudioStreamGain(ctx.hoverStream, *ctx.sfxVolume * GameConst::kHoverStreamGain);
            }

            ImGui::SetCursorPosX(btnX);
            if (musicPaused) {
                if (ImGui::Button("Play Music", ImVec2(btnW, 28.0f))) {
                    SDL_ResumeAudioStreamDevice(ctx.bgmPlayer);
                    musicPaused = false;
                }
            } else {
                if (ImGui::Button("Pause Music", ImVec2(btnW, 28.0f))) {
                    SDL_PauseAudioStreamDevice(ctx.bgmPlayer);
                    musicPaused = true;
                }
            }
            PlayHoverSound();

            // Video
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::SetCursorPosX(btnX);
            ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "VIDEO");

            ImGui::SetCursorPosX(btnX);
            ImGui::Text("Resolution");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(btnW - 100.0f);
            ImGui::Combo("##res", &pendingResIndex,
                SaveData::kResolutionLabels, SaveData::kResolutionCount);

            ImGui::SetCursorPosX(btnX);
            ImGui::Checkbox("Fullscreen", &pendingFullscreen);

            ImGui::SetCursorPosX(btnX);
            ImGui::Text("Sync");
            ImGui::SameLine();
            if (ImGui::RadioButton("Adaptive##vs", pendingVsync == -1)) pendingVsync = -1;
            ImGui::SameLine();
            if (ImGui::RadioButton("VSync##vs",    pendingVsync ==  1)) pendingVsync =  1;
            ImGui::SameLine();
            if (ImGui::RadioButton("Off##vs",      pendingVsync ==  0)) pendingVsync =  0;

            // Frame Cap (only meaningful when Sync is Off)
            ImGui::SetCursorPosX(btnX);
            ImGui::Text("Frame Cap");
            ImGui::SameLine();
            {
                static const char* capLabels[] = { "Uncapped", "240 FPS", "144 FPS", "120 FPS", "60 FPS" };
                static const int   capValues[] = { 0, 240, 144, 120, 60 };
                int capIdx = 0;
                for (int ci = 0; ci < 5; ci++) if (capValues[ci] == pendingTargetFPS) { capIdx = ci; break; }
                ImGui::SetNextItemWidth(btnW - 100.0f);
                if (ImGui::Combo("##cap", &capIdx, capLabels, 5))
                    pendingTargetFPS = capValues[capIdx];
            }

            ImGui::Spacing();
            ImGui::SetCursorPosX(btnX);
            if (ImGui::Button("APPLY VIDEO", ImVec2(btnW, 30))) {
                SaveData::current.resolutionIndex = pendingResIndex;
                SaveData::current.fullscreen      = pendingFullscreen;
                SaveData::current.vsyncMode       = pendingVsync;
                SaveData::current.targetFPS       = pendingTargetFPS;
                int w = SaveData::kResolutionW[pendingResIndex];
                int h = SaveData::kResolutionH[pendingResIndex];
                SceneSwitcher::RequestVideo(pendingFullscreen, w, h, pendingVsync);
            }
            PlayHoverSound();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button("BACK TO TITLE", ImVec2(btnW, 36.0f))) {
            *ctx.gamePaused = false;
            SceneSwitcher::Request(GameScene::TITLE);
        }
        PlayHoverSound();

        ImGui::Spacing();

        ImGui::SetCursorPosX(btnX);
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.50f, 0.10f, 0.10f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.00f, 0.20f, 0.20f, 1.0f));
        if (ImGui::Button("QUIT GAME", ImVec2(btnW, 36.0f))) {
            SDL_Event e{}; e.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&e);
        }
        PlayHoverSound();
        ImGui::PopStyleColor(3);

    ImGui::End();
}

// Game Over screen
void HUDRenderer::DrawGameOver() {
    if (!*ctx.gameOver) return;

    ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(
            ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 220), ImGuiCond_Always);
        ImGui::Begin("##gameover", nullptr,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove   |
            ImGuiWindowFlags_NoTitleBar);

        ImGui::SetWindowFontScale(2.0f);
        ImVec2 goSz = ImGui::CalcTextSize("GAME OVER");
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - goSz.x) * 0.5f);
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "GAME OVER");
        ImGui::SetWindowFontScale(1.0f);

        ImGui::Spacing();
        ImGui::SetCursorPosX(100);
        ImGui::Text("Final Score: %d", *ctx.score);
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::SetCursorPosX(50);
        if (ImGui::Button("Try Again", ImVec2(120, 40))) {
            *ctx.gameOver   = false;
            *ctx.score      = 0;
            *ctx.shardCount = 0;
            ctx.clearShards();
            *ctx.autoSaveTimer = 0.0f;
            *ctx.currentPhase  = 1;
            ctx.player->Reset();
            ctx.asteroid->Reset();
            ctx.bot01->Reset();
            ctx.bot02->Reset();
            ctx.levelDirector->Reset();
            *ctx.prevLives = ctx.player->GetLives();
            SaveData::current.lives    = 3;
            SaveData::current.health   = 100.0f;
            SaveData::current.score    = 0;
            SaveData::current.posX     = 0.0f;
            SaveData::current.posY     = 0.0f;
            SaveData::current.waveTime = 0.0f;
            SaveData::current.shardCount = 0;
            SaveData::current.Save();
        }
        PlayHoverSound();
        ImGui::SameLine();
        if (ImGui::Button("Title", ImVec2(70, 40)))
            SceneSwitcher::Request(GameScene::TITLE);
        PlayHoverSound();
        ImGui::SameLine();
        if (ImGui::Button("Exit", ImVec2(70, 40))) {
            SDL_Event e{}; e.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&e);
        }
        PlayHoverSound();
        ImGui::End();
}
