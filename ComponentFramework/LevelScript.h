#pragma once
#ifndef LEVELSCRIPT_H
#define LEVELSCRIPT_H

#include "LevelEvent.h"
#include <vector>

// Abstract base class for all level chunks.
//
// To add a new level/chunk:
//   1. Create MyLevelScript.h + MyLevelScript.cpp
//   2. Override GetEvents() — return a list of LevelEvents with LOCAL timestamps (start at 0)
//   3. In SceneMuntasir::OnCreate(), call:
//        levelDirector->AddScript(new MyLevelScript(), timeOffset);
//      where timeOffset shifts when this chunk begins on the master timeline.
//
// That's it. No other file needs to change.
class LevelScript {
public:
    virtual ~LevelScript() = default;
    virtual std::vector<LevelEvent> GetEvents() const = 0;
};

#endif // LEVELSCRIPT_H
