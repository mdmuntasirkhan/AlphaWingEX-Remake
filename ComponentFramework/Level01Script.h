#ifndef LEVEL01SCRIPT_H
#define LEVEL01SCRIPT_H

#include "LevelScript.h"

// Opening zone — registered in SceneMuntasir::OnCreate() at time offset 0.
class Level01Script : public LevelScript {
public:
    std::vector<LevelEvent> GetEvents() const override;
};

#endif // LEVEL01SCRIPT_H
