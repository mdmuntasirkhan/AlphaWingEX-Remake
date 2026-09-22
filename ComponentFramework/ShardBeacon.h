#ifndef SHARDBEACON_H
#define SHARDBEACON_H

#include "Mesh.h"
#include "Shader.h"
#include <Matrix.h>
#include <Vector.h>

using namespace MATH;

// Satellite beacon placed at the player's death position when shards are dropped.
// Composed of three Blender exported mesh parts that render together as one object.
// Slowly rotates on the Y-axis and pulses a gold glow so it reads clearly against
// the level background. TryCollect() handles pickup detection each frame.
class ShardBeacon {
private:
    Mesh* dishMesh;
    Mesh* computerMesh;
    Mesh* solarMesh;

    Vec3  pos;
    int   count;
    bool  active;
    float pulseTimer;
    float spinAngle;

    static constexpr float kSpinSpeed = 28.0f;
    static constexpr float kBeaconScale = 0.25f;

public:
    ShardBeacon();
    ~ShardBeacon();

    bool OnCreate();
    void OnDestroy();

    // Drop the beacon at a world position with the given shard count.
    // Calling Place() again while active replaces the previous pile (same as DS rules).
    void Place(const Vec3& position, int shardCount);

    // Deactivate and clear without collection (old beacon replaced by a new death).
    void Clear();

    // Call each frame while the beacon is active.
    void Update(float deltaTime);

    // Check whether the player is close enough to collect. Returns the shard count
    // and deactivates the beacon on success; returns 0 if out of range or inactive.
    int TryCollect(const Vec3& playerPos);

    void Render(Shader* shader,
                const Matrix4& projection,
                const Matrix4& view) const;

    bool IsActive()    const { return active; }
    int  GetCount()    const { return count; }
    Vec3 GetPosition() const { return pos; }

    static constexpr float kPickupRadius = 1.2f;
};

#endif // SHARDBEACON_H
