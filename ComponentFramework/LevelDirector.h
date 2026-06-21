#pragma once
#ifndef LEVELDIRECTOR_H
#define LEVELDIRECTOR_H

#include "LevelEvent.h"
#include "LevelScript.h"
#include "Mesh.h"
#include "Shader.h"
#include <Matrix.h>
#include <vector>
#include <map>
#include <string>
#include <functional>

using namespace MATH;

// Runs the master level timeline and renders all active environment chunks.
//
// Responsibilities:
//   - Merges level script chunks into one sorted event list
//   - Pre-loads every referenced .obj mesh at startup (no runtime stutter)
//   - Fires events as levelTime advances in Update()
//   - Scrolls active chunks left and culls them when off-screen
//   - Draws them through the shared shader in Render()
//
// It does NOT touch enemies, bullets, the player, or save data.
class LevelDirector {
public:
    LevelDirector();
    ~LevelDirector() = default;

    // Merge a script chunk into the master timeline.
    // timeOffset shifts all of the script's local timestamps to absolute time,
    // allowing chunks to overlap seamlessly (Elden-ring style, no dead zones).
    // Also pre-loads every mesh the script references. Takes ownership of script.
    void AddScript(LevelScript* script, float timeOffset = 0.0f);

    // Register a callback invoked whenever a PHASE_CHANGE event fires.
    // SceneMuntasir calls this once after construction to drive enemy phase logic.
    void SetPhaseCallback(std::function<void(int)> cb);

    // Current position on the master timeline (seconds).
    float GetTime() const;

    // Call once per frame (not while paused).
    void Update(float deltaTime);

    // Call inside SceneMuntasir::Render() — draws all currently visible chunks.
    void Render(Shader* shader,
                const Matrix4& projectionMatrix,
                const Matrix4& viewMatrix) const;

    // Release all loaded meshes. Call before delete.
    void OnDestroy();

private:
    // A mesh chunk currently scrolling through the scene
    struct ActiveChunk {
        Vec3   pos;
        Mesh*  mesh        = nullptr;
        Vec3   color;
        float  scale       = 0.0f;
        float  scrollSpeed = 0.0f;
    };

    float                        levelTime;
    int                          nextEvent;
    std::vector<LevelEvent>      timeline;
    std::vector<ActiveChunk>     activeChunks;
    std::map<std::string, Mesh*> meshPool;
    std::function<void(int)>     phaseCallback;

    void FireEvent(const LevelEvent& e);
};

#endif // LEVELDIRECTOR_H
