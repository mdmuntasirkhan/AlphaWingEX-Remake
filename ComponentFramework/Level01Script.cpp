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

        // ============================================================
        //  ASTEROIDS
        //  SET_ASTEROID_RATE — scale=large spawn interval (s), scrollSpeed=small spawn interval (s)
        //  Lower number = more asteroids. Change mid-level to ramp up density.
        // ============================================================
        { 0.0f, EventType::SET_ASTEROID_RATE, {}, nullptr, {}, 3.2f, 2.0f, 0 }, // opening density

        // ============================================================
        //  BOT01 WAVES
        //  SPAWN_BOT01_GROUP    — standard bots (no shield)
        //  SPAWN_BOT01_SHIELDED — shielded bots (activate shield when missile is near)
        //  scale       = number of bots to spawn
        //  scrollSpeed = seconds between each individual spawn (0 = all at once)
        // ============================================================
        { 40.0f,  EventType::SPAWN_BOT01_GROUP,    {}, nullptr, {}, 5.0f, 10.0f, 0 }, // 5 bots, one every 10 s
        { 95.0f,  EventType::SPAWN_BOT01_SHIELDED, {}, nullptr, {}, 1.0f,  0.0f, 0 }, // 1 shielded bot
        { 145.0f, EventType::SPAWN_BOT01_GROUP,    {}, nullptr, {}, 3.0f,  8.0f, 0 }, // 3 bots, one every 8 s
        { 165.0f, EventType::SPAWN_BOT01_SHIELDED, {}, nullptr, {}, 1.0f,  0.0f, 0 }, // 1 shielded bot

        // ============================================================
        //  BOT02
        //  SPAWN_BOT02 — spawns a Bot02 pair at the given time.
        //  Pair stays until both are killed. Add another SPAWN_BOT02
        //  below if you want a second appearance later in the level.
        //  The PHASE_CHANGE events at 115/140 pause Bot01+asteroids
        //  during the Bot02 intro window — adjust them together.
        // ============================================================
        { 115.0f, EventType::PHASE_CHANGE, {}, nullptr, {}, 0.0f, 0.0f, 3 }, // pause Bot01+asteroids
        { 115.0f, EventType::SPAWN_BOT02,  {}, nullptr, {}, 0.0f, 0.0f, 0 }, // Bot02 appears
        { 140.0f, EventType::PHASE_CHANGE, {}, nullptr, {}, 0.0f, 0.0f, 4 }, // resume Bot01+asteroids

        // ============================================================
        //  ENVIRONMENT GEOMETRY
        //  SPAWN_ENV_CHUNK — a Blender OBJ mesh that scrolls left through the scene.
        //  position.y  = vertical spawn position (±6 = top/bottom of screen)
        //  color       = Vec3(R, G, B) tint   scale = visual size   scrollSpeed = units/s leftward
        // ============================================================
        // { 5.0f, EventType::SPAWN_ENV_CHUNK, Vec3(15.0f, 0.0f, -10.0f), "meshes/level01_rock.obj", Vec3(0.5f, 0.5f, 0.6f), 1.0f, 1.5f },

    };
}
