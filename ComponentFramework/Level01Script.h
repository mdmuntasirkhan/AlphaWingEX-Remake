#pragma once
#ifndef LEVEL01SCRIPT_H
#define LEVEL01SCRIPT_H

#include "LevelScript.h"

// Level chunk 1 — opening zone.
// Define your environment meshes (exported from Blender) here in Level01Script.cpp.
// Registered in SceneMuntasir::OnCreate() at time offset 0.
class Level01Script : public LevelScript {
public:
    std::vector<LevelEvent> GetEvents() const override;
};

#endif // LEVEL01SCRIPT_H
