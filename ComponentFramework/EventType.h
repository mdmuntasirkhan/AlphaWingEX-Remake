#pragma once
#ifndef EVENTTYPE_H
#define EVENTTYPE_H

// All event types the level director can fire.
// Add new types here as the engine grows (enemy triggers, music changes, etc.)
enum class EventType {
    SPAWN_ENV_CHUNK,    // spawn a Blender-exported mesh that scrolls through the level
};

#endif // EVENTTYPE_H
