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
    SPACE,  // normal space gameplay — default
    WATER,  // underwater zone: faster star scroll, random Y jitter on stars
            // NOTE: GetJitterX/Y() exist for future player physics coupling — not yet wired in
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

    // Hyperspace warp effect
    enum class WarpMode { FULL, ENTER, EXIT };
    WarpMode warpMode;
    bool     warpActive;
    float    warpTimer;
    float    warpDuration;
    float    warpSpeed;         // current star speed multiplier (1.0 = normal)
    float    postWarpSpeed;     // speed at the moment EXIT warp ended
    float    postWarpCooldown;  // seconds remaining in post-exit deceleration

public:
    Environment();
    ~Environment();

    bool OnCreate(float width, float height);
    void OnResize(float width, float height); // call from SceneMuntasir::OnVideoChanged
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

    // WARP_ENTER — scene opens already at peak warp; smoothly decelerates to normal.
    void TriggerWarpEnter(float duration = 10.0f);
    // WARP_EXIT  — starts at normal speed; smoothly accelerates to peak warp.
    void TriggerWarpExit(float duration = 10.0f);
    // WARP_FULL  — full 3-phase: ramp-up → hold → ramp-down (F11 test).
    void TriggerWarp(float duration = 10.0f);
    bool IsWarpActive() const { return warpActive; }
    bool IsFullWarp()   const { return warpActive && warpMode == WarpMode::FULL; }
};

#endif