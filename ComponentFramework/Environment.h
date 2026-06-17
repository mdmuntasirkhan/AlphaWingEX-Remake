#pragma once
#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <Vector.h>
#include <Matrix.h>
#include "Shader.h"
#include "imgui.h"
#include <vector>

using namespace MATH;

enum class EnvironmentType {
    SPACE,   // normal gameplay
    WATER,   // slow movement, jittery controls
};

struct Star {
    float x, y;
    float speed;
    float size;
};


class Environment {
private:
    EnvironmentType currentType;

    // Starfield
    std::vector<Star> stars;
    int starCount;


    // Screen dimensions
    float screenWidth;
    float screenHeight;

    // Water effect
    float waterJitterTimer;

public:
    Environment();
    ~Environment();

    bool OnCreate(float width, float height);
    void OnDestroy();
    void Update(float deltaTime);
    void Render() const;  // uses ImGui background draw list

    // Getters
    EnvironmentType GetType() const { return currentType; }
    void SetType(EnvironmentType type) { currentType = type; }

    // Player modifier - returns speed multiplier
    float GetSpeedMultiplier() const;

    // Water jitter - returns small random offset
    float GetJitterX() const;
    float GetJitterY() const;
};

#endif