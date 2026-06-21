#include "Level01Script.h"
#include "EventType.h"
#include <Vector.h>

// ============================================================
//  LEVEL 01 — Opening zone
//
//  HOW TO ADD YOUR BLENDER MESH:
//    1. Export your mesh from Blender as OBJ → save to the meshes/ folder
//    2. Add a line here: { time, SPAWN_ENV_CHUNK, Vec3(x, y, -10), "meshes/yourfile.obj", color, scale, speed }
//    3. Build and run — the chunk will scroll into the scene at the time you specified.
//
//  COLUMNS:
//    time        — seconds from level start when the chunk spawns at X=15 (right edge)
//    position    — Vec3(15, Y, -10) : change Y to place it top/middle/bottom of screen
//    meshFile    — path to your .obj file (relative to the game exe)
//    color       — Vec3(R, G, B)  range 0.0–1.0
//    scale       — 1.0 = natural size; tweak until it looks right in game
//    scrollSpeed — 1.5 matches the asteroid field feel; lower = slower/bigger feeling
//
//  The placeholder events below use the asteroid mesh so you can see the system
//  working immediately. Replace them with your own Blender exports.
// ============================================================

std::vector<LevelEvent> Level01Script::GetEvents() const {
    return {
        // ---- Phase gates: drive enemy progression from the level script ----
        // Phase 1 (t=0): asteroids only — implicit, no event needed (currentPhase starts at 1)
        // Phase 2 (t=40): Bot01 joins the asteroid field
        { 40.0f,  EventType::PHASE_CHANGE, {}, nullptr, {}, 0.0f, 0.0f, 2 },
        // Phase 3 (t=115): Bot02 intro window — Bot01 and asteroids pause
        { 115.0f, EventType::PHASE_CHANGE, {}, nullptr, {}, 0.0f, 0.0f, 3 },
        // Phase 4 (t=140): All enemies active simultaneously
        { 140.0f, EventType::PHASE_CHANGE, {}, nullptr, {}, 0.0f, 0.0f, 4 },

        // ---- Environment geometry (hollow — add Blender .obj exports here) ----
        // Format: { time, SPAWN_ENV_CHUNK, Vec3(15, Y, -10), "meshes/file.obj", Vec3(R,G,B), scale, speed }
        // Example: { 5.0f, EventType::SPAWN_ENV_CHUNK, Vec3(15.0f, 0.0f, -10.0f), "meshes/level01_rock.obj", Vec3(0.5f, 0.5f, 0.6f), 1.0f, 1.5f },
    };
}
