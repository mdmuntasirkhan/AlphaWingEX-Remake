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

// Runs the master level timeline — merges scripts, pre-loads meshes,
// fires timed events, scrolls and culls active env chunks.
// Does not touch enemies, bullets, the player, or save data.
class LevelDirector {
private:
    // A Blender mesh chunk currently scrolling through the scene
    struct ActiveChunk {
        Vec3   pos;
        Mesh* mesh           = nullptr;
        Vec3   color;
        float  scale         = 0.0f;
        float  scrollSpeed   = 0.0f;
    };

    float                        levelTime;
    int                          nextEvent;
    bool                         warpEnterRequested;
    bool                         warpExitRequested;
    bool                         warpFullRequested;

    std::vector<LevelEvent>      timeline;
    std::vector<ActiveChunk>     activeChunks;
    std::map<std::string, Mesh*> meshPool;                        // one mesh loaded per unique file path

    // Callbacks — wired by SceneMuntasir after construction
    std::function<void(int)>                 phaseCallback;       // PHASE_CHANGE
    std::function<void(int, float, bool)>    bot01Callback;       // SPAWN_BOT01 (count, interval, shielded)
    std::function<void()>                    bot02Callback;       // SPAWN_BOT02
    std::function<void(float, float)>        asteroidCallback;    // SET_ASTEROID_RATE (large, small)

    void FireEvent(const LevelEvent& e);

public:
    LevelDirector();
    ~LevelDirector() = default;

    // Merges a script into the master timeline at the given time offset.
    // Pre-loads every mesh the script references. Takes ownership of script.
    void AddScript(LevelScript* script, float timeOffset = 0.0f);

    // Callbacks — call once after construction before the first Update()
    void SetPhaseCallback(std::function<void(int)> cb);
    void SetBot01Callback(std::function<void(int, float, bool)> cb);        // (count, interval, shielded)
    void SetBot02Callback(std::function<void()> cb);
    void SetAsteroidCallback(std::function<void(float, float)> cb);         // (large interval, small interval)

    // Resets timeline to t=0 for Try Again — loaded meshes are kept
    void Reset();

    // Each returns true once when the matching warp event fires
    bool PopWarpEnterRequest() { bool v = warpEnterRequested; warpEnterRequested = false; return v; }
    bool PopWarpExitRequest()  { bool v = warpExitRequested;  warpExitRequested  = false; return v; }
    bool PopWarpFullRequest()  { bool v = warpFullRequested;  warpFullRequested  = false; return v; }

    float GetTime() const;

    void Update(float deltaTime);
    void Render(Shader* shader,
                const Matrix4& projectionMatrix,
                const Matrix4& viewMatrix) const;
    void OnDestroy();
};

#endif // LEVELDIRECTOR_H
