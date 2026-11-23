#ifndef EVENTTYPE_H
#define EVENTTYPE_H

// All event types the level director can fire.
// Add new types here as the engine grows (enemy triggers, music changes, etc.)
enum class EventType {
    SPAWN_ENV_CHUNK,      // spawn a Blender exported mesh that scrolls through the level
    WARP_ENTER,           // spawn animation  : starts at peak warp speed, decelerates to normal
    WARP_EXIT,            // transition out   : starts at normal speed, accelerates to peak warp
    WARP_FULL,            // full 3 phase warp: ramp up > hold > ramp down (Hold Q to test)
    PHASE_CHANGE,         // advances enemy phase — uses phaseId field in LevelEvent
    SPAWN_BOT01_GROUP,    // standard wave   — scale=count, scrollSpeed=spawn interval (s)
    SPAWN_BOT01_SHIELDED, // shielded wave   — scale=count, scrollSpeed=spawn interval (s)
    SPAWN_BOT02,          // spawns top + bottom pair — no extra fields needed
    SET_ASTEROID_RATE,    // scale=large interval (s), scrollSpeed=small interval (s)
};

#endif // EVENTTYPE_H
