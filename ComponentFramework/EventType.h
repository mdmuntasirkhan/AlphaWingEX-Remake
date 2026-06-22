#pragma once
#ifndef EVENTTYPE_H
#define EVENTTYPE_H

// All event types the level director can fire.
// Add new types here as the engine grows (enemy triggers, music changes, etc.)
enum class EventType {
    SPAWN_ENV_CHUNK,    // spawn a Blender-exported mesh that scrolls through the level
    WARP_START,         // trigger a 10-second hyperspace jump animation in Environment
    PHASE_CHANGE,       // advance enemy progression phase (phaseId field in LevelEvent)
    SPAWN_BOT01_GROUP,    // standard Bot01 wave  — scale=count (int cast), scrollSpeed=seconds between spawns
    SPAWN_BOT01_SHIELDED, // shielded Bot01 wave  — scale=count (int cast, usually 1), scrollSpeed=seconds between spawns
    SPAWN_BOT02,          // spawn a Bot02 pair   — no extra fields needed
    SET_ASTEROID_RATE,    // change asteroid spawn cadence — scale=large interval (s), scrollSpeed=small interval (s)
};

#endif // EVENTTYPE_H
