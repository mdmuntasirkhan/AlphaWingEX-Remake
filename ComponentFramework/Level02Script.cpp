#include "Level02Script.h"
#include "EventType.h"
#include <Vector.h>

// ============================================================
//  LEVEL 02 — Second zone (Blender mesh zone)
//
//  HOW TO ADD YOUR BLENDER MESH:
//    1. Export from Blender as OBJ → save to meshes/
//    2. Add a line: { time, SPAWN_ENV_CHUNK, Vec3(15, Y, -10), "meshes/file.obj", color, scale, speed }
//    3. Build and run.
//
//  COLUMNS:
//    time        — seconds from level START (t=0), not from this chunk's offset.
//                  LevelDirector shifts these by the timeOffset you pass to AddScript().
//    position    — Vec3(15, Y, -10) : Y places it top/middle/bottom of screen
//    meshFile    — path to your .obj (relative to the game exe)
//    color       — Vec3(R, G, B)  range 0.0–1.0
//    scale       — 1.0 = natural size
//    scrollSpeed — 1.5 matches asteroid feel; lower = slower/bigger
//
//  SEAMLESS TRANSITION:
//    The timeOffset in AddScript() (currently 180.0f) controls when Level02 chunks
//    first appear. Lower it by ~22s to overlap with Level01's last chunks on screen.
// ============================================================

std::vector<LevelEvent> Level02Script::GetEvents() const {
    return {
        // Add your Blender mesh events here. Example:
        // { 0.0f, EventType::SPAWN_ENV_CHUNK, Vec3(15.0f, 0.0f, -10.0f), "meshes/level02_hull.obj", Vec3(0.3f, 0.5f, 0.8f), 1.0f, 1.5f },
    };
}
