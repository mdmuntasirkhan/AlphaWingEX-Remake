#pragma once
#ifndef LEVELEVENT_H
#define LEVELEVENT_H

#include "EventType.h"
#include <Vector.h>

using namespace MATH;

// One timed instruction in a level script.
// Fill these in your LevelXXScript.cpp to define WHAT spawns, WHEN, and WHERE.
//
// Field meanings per event type — the same struct fields are reused with different
// semantics depending on `type`. Read the column for your event type:
//
//  EventType             | position          | meshFile  | color       | scale              | scrollSpeed          | phaseId
//  ----------------------|-------------------|-----------|-------------|--------------------|-----------------------|--------
//  SPAWN_ENV_CHUNK       | world spawn pos   | .obj path | RGB tint    | uniform mesh scale | units/s leftward      | unused
//  PHASE_CHANGE          | unused            | unused    | unused      | unused             | unused                | phase ID
//  SPAWN_BOT01_GROUP     | unused            | unused    | unused      | bot count (int)    | seconds between bots  | unused
//  SPAWN_BOT01_SHIELDED  | unused            | unused    | unused      | bot count (int)    | seconds between bots  | unused
//  SPAWN_BOT02           | unused            | unused    | unused      | unused             | unused                | unused
//  SET_ASTEROID_RATE     | unused            | unused    | unused      | large spawn (s)    | small spawn (s)       | unused
//  WARP_START            | unused            | unused    | unused      | unused             | unused                | unused
//
struct LevelEvent {
    float       time        = 0.0f;                      // seconds since level start when this fires
    EventType   type        = EventType::SPAWN_ENV_CHUNK;
    Vec3        position    = Vec3(15.0f, 0.0f, -10.0f); // world spawn position (X=15 = right edge)
    const char* meshFile    = nullptr;                    // .obj path (SPAWN_ENV_CHUNK only)
    Vec3        color       = Vec3(0.7f, 0.7f, 0.8f);   // RGB tint (SPAWN_ENV_CHUNK only)
    float       scale       = 1.0f;                      // see table above
    float       scrollSpeed = 1.5f;                      // see table above
    int         phaseId     = 0;                         // PHASE_CHANGE only
};

#endif // LEVELEVENT_H
