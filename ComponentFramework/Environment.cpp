#include "Environment.h"
#include <cstdlib>
#include <cmath>

Environment::Environment() :
    currentType{ EnvironmentType::SPACE },
    starCount{ 150 },
    screenWidth{ 1280.0f },
    screenHeight{ 720.0f },
    waterJitterTimer{ 0.0f } {
}

Environment::~Environment() {}

bool Environment::OnCreate(float width, float height) {
    screenWidth = width;
    screenHeight = height;

    // Generate random stars
    stars.clear();
    for (int i = 0; i < starCount; i++) {
        Star star;
        star.x = (float)(rand() % (int)screenWidth);
        star.y = (float)(rand() % (int)screenHeight);
        star.speed = 20.0f + (rand() % 80);   // random scroll speed
        star.size = 0.5f + (rand() % 2);       // random size
        stars.push_back(star);
    }
    return true;
}

void Environment::OnDestroy() {
    stars.clear();
}

void Environment::Update(float deltaTime) {
    // Scroll stars from right to left
    for (int i = 0; i < stars.size(); i++) {
        stars[i].x -= stars[i].speed * deltaTime;

        // Wrap around when off screen
        if (stars[i].x < 0.0f) {
            stars[i].x = screenWidth;
            stars[i].y = (float)(rand() % (int)screenHeight);
        }
    }

    // Water jitter timer
    if (currentType == EnvironmentType::WATER) {
        waterJitterTimer += deltaTime;
    }
}

void Environment::Render() const {
    // Draw starfield using ImGui background draw list
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    for (int i = 0; i < stars.size(); i++) {
        // Dimmer stars further back, brighter ones in front
        int brightness = 150 + (int)(stars[i].speed);
        if (brightness > 255) brightness = 255;

        drawList->AddCircleFilled(
            ImVec2(stars[i].x, stars[i].y),
            stars[i].size,
            IM_COL32(brightness, brightness, brightness, 255)
        );
    }

    // Water overlay - blue tint at bottom of screen
    if (currentType == EnvironmentType::WATER) {
        drawList->AddRectFilled(
            ImVec2(0, screenHeight * 0.6f),
            ImVec2(screenWidth, screenHeight),
            IM_COL32(0, 50, 150, 80)  // semi transparent blue
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