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
    WATER,  // underwater zone: faster star scroll, random Y jitter on stars - not implimented yet
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
    float screenWidth;
    float screenHeight;

    // Stars between these two screen rows (pixels from the top) are not drawn. The boss arena floor uses it.
    float starMaskTop;
    float starMaskBottom;
    float starMaskLeft;       // the mask only covers stars between these two x values (pixels)
    float starMaskRight;
    float starMask2Top;       // a second mask rectangle (the corridor hull), same meaning as the first
    float starMask2Bottom;
    float starMask2Left;
    float starMask2Right;

    // Water effect
    float waterJitterTimer;

    // Hyperspace warp
    enum class WarpMode { FULL, ENTER, EXIT };
    WarpMode warpMode;
    bool     warpActive;
    float    warpTimer;
    float    warpDuration;
    float    warpSpeed;         // current star speed multiplier (1.0 = normal)
    float    postWarpSpeed;     // speed at the moment EXIT warp ended
    float    postWarpCooldown;  // seconds remaining in post exit deceleration

public:
    Environment();
    ~Environment();

    bool OnCreate(float width, float height);
    void OnResize(float width, float height); // call from SceneMuntasir::OnVideoChanged
    void OnDestroy();
    void Update(float deltaTime);
    // Stars inside this band are not drawn. Both 0 means no mask. leftX and rightX limit it sideways (pixels).
    void SetStarMask(float topY, float bottomY, float leftX = -1.0e9f, float rightX = 1.0e9f) {
        starMaskTop = topY; starMaskBottom = bottomY; starMaskLeft = leftX; starMaskRight = rightX;
        starMask2Top = 0.0f; starMask2Bottom = 0.0f;   // the second mask is off until SetStarMask2 is called
    }
    // A second rectangle, for the hull of the arena corridor. Call it after SetStarMask each frame (SetStarMask turns it off).
    void SetStarMask2(float topY, float bottomY, float leftX, float rightX) {
        starMask2Top = topY; starMask2Bottom = bottomY; starMask2Left = leftX; starMask2Right = rightX;
    }
    void Render() const;

    // Getters
    EnvironmentType GetType() const { return currentType; }
    void SetType(EnvironmentType type) { currentType = type; }

    // Player modifier
    float GetSpeedMultiplier() const;

    // Water jitter - returns small random offset
    float GetJitterX() const;
    float GetJitterY() const;

    // WARP_ENTER
    void TriggerWarpEnter(float duration = 10.0f);
    // WARP_EXIT
    void TriggerWarpExit(float duration = 10.0f);
    // WARP_FULL
    void TriggerWarp(float duration = 10.0f);

    bool IsWarpActive() const { return warpActive; }
    bool IsFullWarp()   const { return warpActive && warpMode == WarpMode::FULL; }
};

#endif // ENVIRONMENT_H