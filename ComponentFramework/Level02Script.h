#ifndef LEVEL02SCRIPT_H
#define LEVEL02SCRIPT_H

#include "LevelScript.h"

// Second zone — registered in SceneMuntasir::OnCreate() at time offset 180.0f (3 min).
//
// Seamless transition: Level01 ends around t=180. To overlap chunks from both levels
// simultaneously, lower the timeOffset by up to 22s (one screen transit at scrollSpeed=1.5f).
//
// To add a Blender mesh:
//   1. Export OBJ from Blender → save to meshes/
//   2. Add: { time, SPAWN_ENV_CHUNK, Vec3(15, Y, -10), "meshes/file.obj", color, scale, speed }
class Level02Script : public LevelScript {
public:
    std::vector<LevelEvent> GetEvents() const override;
};

#endif // LEVEL02SCRIPT_H
