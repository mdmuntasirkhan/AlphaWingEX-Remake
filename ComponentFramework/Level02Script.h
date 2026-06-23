#pragma once
#ifndef LEVEL02SCRIPT_H
#define LEVEL02SCRIPT_H

#include "LevelScript.h"

// Level chunk 2 — second zone.
// Registered in SceneMuntasir::OnCreate() at timeOffset=180.0f (3 min into play).
//
// SEAMLESS TRANSITION NOTE:
//   Level01 ends around t=180. Level02 is queued at t=180, so its first env chunk
//   enters from the right exactly as Level01's last chunk exits left. To push the
//   overlap window earlier (chunks from both levels on screen simultaneously), lower
//   the timeOffset passed to AddScript() in SceneMuntasir::OnCreate().
//   At scrollSpeed=1.5f the screen transit time is ~22s, so offsetting by -22 gives
//   a full-screen crossfade between the two zones.
//
// HOW TO ADD YOUR BLENDER MESH:
//   1. Export from Blender as OBJ → save to meshes/
//   2. Add an event: { time, SPAWN_ENV_CHUNK, Vec3(15, Y, -10), "meshes/file.obj", color, scale, speed }
//   3. Build and run — no other file needs to change.
class Level02Script : public LevelScript {
public:
    std::vector<LevelEvent> GetEvents() const override;
};

#endif // LEVEL02SCRIPT_H
