#include "Environment.h"
#include <cstdlib>
#include <cmath>

Environment::Environment() :
    currentType { EnvironmentType::SPACE },
    starCount   { 150 },
    screenWidth { 1920.0f },
    screenHeight{ 1080.0f },
    waterJitterTimer{ 0.0f },
    warpMode    { WarpMode::FULL },
    warpActive  { false },
    warpTimer   { 0.0f  },
    warpDuration{ 10.0f },
    warpSpeed   { 1.0f  } {
}

Environment::~Environment() {}

bool Environment::OnCreate(float width, float height) {
    screenWidth  = width;
    screenHeight = height;
    // Scale star count proportional to screen area so density stays constant
    starCount = (int)(150 * (width * height) / (1920.0f * 1080.0f));
    if (starCount < 80)  starCount = 80;
    if (starCount > 800) starCount = 800;
    stars.clear();
    for (int i = 0; i < starCount; i++) {
        Star star;
        star.x     = (float)(rand() % (int)screenWidth);
        star.y     = (float)(rand() % (int)screenHeight);
        star.speed = 20.0f + (rand() % 80);
        star.size  = 0.5f + (rand() % 2);
        stars.push_back(star);
    }
    return true;
}

void Environment::OnResize(float width, float height) {
    OnCreate(width, height);
}

void Environment::OnDestroy() {
    stars.clear();
}

// Smoothstep: zero-derivative at both ends — feels buttery, no pop.
static inline float smoothstep(float p) {
    return p * p * (3.0f - 2.0f * p);
}

void Environment::TriggerWarpEnter(float duration) {
    warpMode     = WarpMode::ENTER;
    warpActive   = true;
    warpTimer    = 0.0f;
    warpDuration = duration;
    warpSpeed    = 40.0f;   // already at peak — scene opens mid-warp
}

void Environment::TriggerWarpExit(float duration) {
    warpMode     = WarpMode::EXIT;
    warpActive   = true;
    warpTimer    = 0.0f;
    warpDuration = duration;
    warpSpeed    = 1.0f;    // starts at normal; accelerates to peak
}

void Environment::TriggerWarp(float duration) {
    warpMode     = WarpMode::FULL;
    warpActive   = true;
    warpTimer    = 0.0f;
    warpDuration = duration;
    warpSpeed    = 1.0f;
}

void Environment::Update(float deltaTime) {
    if (warpActive) {
        warpTimer += deltaTime;
        float t = warpTimer / warpDuration;   // 0..1

        if (t >= 1.0f) {
            warpActive = false;
            warpSpeed  = 1.0f;
        } else {
            switch (warpMode) {

            case WarpMode::ENTER:
                // Scene opens at peak; smoothly decelerates to normal over full duration.
                warpSpeed = 40.0f - smoothstep(t) * 39.0f;   // 40 → 1
                break;

            case WarpMode::EXIT:
                // Starts at normal; smoothly accelerates to peak over full duration.
                warpSpeed = 1.0f + smoothstep(t) * 39.0f;    // 1 → 40
                break;

            case WarpMode::FULL:
            default:
                // 3 phases: ramp-up (30%) → hold (40%) → ramp-down (30%), all smoothstepped.
                if (t < 0.3f) {
                    float p   = t / 0.3f;
                    warpSpeed = 1.0f + smoothstep(p) * 39.0f;  // 1 → 40
                } else if (t < 0.7f) {
                    warpSpeed = 40.0f;
                } else {
                    float p   = (t - 0.7f) / 0.3f;
                    warpSpeed = 40.0f - smoothstep(p) * 39.0f; // 40 → 1
                }
                break;
            }
        }
    }

    // Scroll stars — warpSpeed multiplies each star's base speed
    for (int i = 0; i < (int)stars.size(); i++) {
        stars[i].x -= stars[i].speed * warpSpeed * deltaTime;

        if (stars[i].x < 0.0f) {
            stars[i].x = screenWidth;
            stars[i].y = (float)(rand() % (int)screenHeight);
        }
    }

    if (currentType == EnvironmentType::WATER) {
        waterJitterTimer += deltaTime;
    }
}

void Environment::Render() const {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    bool streaking = warpActive && warpSpeed > 3.0f;

    for (int i = 0; i < (int)stars.size(); i++) {
        int brightness = 150 + (int)(stars[i].speed);
        if (brightness > 255) brightness = 255;

        if (streaking) {
            // Streak length grows with speed; cap at 120 px
            float streakLen = (warpSpeed - 3.0f) * stars[i].size * 6.0f;
            if (streakLen > 120.0f) streakLen = 120.0f;

            // Colour bleaches toward blue-white as speed peaks
            float w = (warpSpeed - 3.0f) / 37.0f;
            if (w > 1.0f) w = 1.0f;
            int r = brightness + (int)((255 - brightness) * w * 0.6f);
            int g = brightness + (int)((255 - brightness) * w * 0.6f);
            int b = 255;
            if (r > 255) r = 255;
            if (g > 255) g = 255;

            // Draw streak left (stars fly toward the player)
            drawList->AddLine(
                ImVec2(stars[i].x, stars[i].y),
                ImVec2(stars[i].x + streakLen, stars[i].y),
                IM_COL32(r, g, b, 255),
                stars[i].size
            );
        } else {
            drawList->AddCircleFilled(
                ImVec2(stars[i].x, stars[i].y),
                stars[i].size,
                IM_COL32(brightness, brightness, brightness, 255)
            );
        }
    }

    if (currentType == EnvironmentType::WATER) {
        drawList->AddRectFilled(
            ImVec2(0, screenHeight * 0.6f),
            ImVec2(screenWidth, screenHeight),
            IM_COL32(0, 50, 150, 80)
        );
    }
}

float Environment::GetSpeedMultiplier() const {
    switch (currentType) {
    case EnvironmentType::WATER:
        return 0.4f;  // 40% speed in water
    case EnvironmentType::SPACE:
    default:
        return 1.0f;  // full speed in space
    }
}

float Environment::GetJitterX() const {
    if (currentType == EnvironmentType::WATER) {
        return ((rand() % 3) - 1) * 0.02f;
    }
    return 0.0f;
}

float Environment::GetJitterY() const {
    if (currentType == EnvironmentType::WATER) {
        return ((rand() % 3) - 1) * 0.02f;
    }
    return 0.0f;
}