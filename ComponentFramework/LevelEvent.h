#pragma once
#ifndef LEVELEVENT_H
#define LEVELEVENT_H

#include "EventType.h"
#include <Vector.h>

using namespace MATH;

// One timed instruction in a level script.
// Fill these in your LevelXXScript.cpp to define WHAT spawns, WHEN, and WHERE.
struct LevelEvent {
    float       time        = 0.0f;                      // seconds since level start
    EventType   type        = EventType::SPAWN_ENV_CHUNK;
    Vec3        position    = Vec3(15.0f, 0.0f, -10.0f); // world spawn position (X=15 = right edge)
    const char* meshFile    = nullptr;                   // path to .obj exported from Blender
    Vec3        color       = Vec3(0.7f, 0.7f, 0.8f);   // RGB tint applied via Phong shader
    float       scale       = 1.0f;                      // uniform scale
    float       scrollSpeed = 1.5f;                      // units/second moving left (matches asteroid feel)
};

#endif // LEVELEVENT_H
