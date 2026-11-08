#ifndef LEVELSCRIPT_H
#define LEVELSCRIPT_H

#include "LevelEvent.h"
#include <vector>

// To add a new level chunk:
//   1. Create MyLevelScript.h + MyLevelScript.cpp
//   2. Override GetEvents() — return LevelEvents with local timestamps starting at 0
//   3. In SceneMuntasir::OnCreate(): levelDirector->AddScript(new MyLevelScript(), timeOffset)
// No other file needs to change.

class LevelScript {
public:
    virtual ~LevelScript() = default;
    virtual std::vector<LevelEvent> GetEvents() const = 0;
};

#endif // LEVELSCRIPT_H
