#ifndef BOT02_H
#define BOT02_H

#include "Enemy.h"
#include <vector>

// Bot02 always spawns as two units "top + bottom" and a fixed hover position on the right side, then fires aimed slow bullets at the player.
class Bot02 : public Enemy {
private:
    // Meshes
    Mesh* bodyMesh;
    Mesh* cockpitMesh;
    Mesh* finMesh;
    Mesh* thrustMesh;
    Mesh* fragmentMesh;
    Mesh* bulletMesh;
    float thrustTimer;
    float hoverTimer;

    // Per instance data, all vectors stay in sync by index (always 0 or 2 active)
    std::vector<Vec3>  positions;
    std::vector<Vec3>  targetPositions; // fixed hover point each instance glides toward
    std::vector<float> hoverPhases;     // offset so top and bottom oscillate out of syn
    std::vector<int>   hp;
    std::vector<float> hitTimers;       // white flash duration after taking a hit
    std::vector<float> fireTimers;      // counts up to kFireInterval then fires
    std::vector<float> knockVelX;       // knockback
    std::vector<float> knockVelY;

    // Bullet pool
    std::vector<Vec3>  bulletPositions;
    std::vector<Vec3>  bulletVelocities;
    std::vector<bool>  bulletReflected;

    // Tuning constants
    static constexpr float kScale           = 0.17f; // uniform render scale (matches Bot01)
    static constexpr float kKnockbackDecay  = 9.0f;  // exponential decay rate for knockback
    static constexpr float kHoverX         = 6.0f;
    static constexpr float kHoverYOffset   = 3.0f;
    static constexpr float kApproachSpeed  = 3.0f;
    static constexpr float kHoverAmplitude = 0.15f;
    static constexpr float kHoverFrequency = 1.8f;
    static constexpr int   kHP             = 20;
    static constexpr float kFireInterval   = 3.0f;  // seconds between per shots
    static constexpr float kBulletSpeed    = 3.0f;  // slow but precise
    static constexpr float kBulletScale    = 0.08f;

public:
    Bot02();
    ~Bot02();

    // Lifecycle
    bool OnCreate(const char* bodyFile, const char* cockpitFile,
                  const char* finFile,  const char* thrustFile,
                  const char* fragmentFile, const char* bulletFile);
    void OnDestroy() override;
    void Update(float deltaTime, float playerX = 0.0f, float playerY = 0.0f) override;
    void Render(Shader* shader, const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const override;
    void Reset() override;

    // Spawn
    void Spawn(float playerY);

    // Getters for collision check in sceneMuntasir
    std::vector<Vec3>& GetPositions()       { return positions; }
    std::vector<Vec3>& GetBulletPositions() { return bulletPositions; }
    std::vector<Vec3>& GetBulletVelocities(){ return bulletVelocities; }
    int GetCount() const { return (int)positions.size(); }

    // Damage and removal
    bool DamageBot02(int index, int amount = 1);
    void RemoveBot02(int index);
    void RemoveBullet(int index);

    // Bullet reflection, called by shield collision in sceneMuntasir
    void MarkBulletReflected(int index) { if (index >= 0 && index < (int)bulletReflected.size()) bulletReflected[index] = true; }
    bool IsBulletReflected(int index)   const { return index >= 0 && index < (int)bulletReflected.size() && bulletReflected[index]; }
    
    // Knockback
    void PushBot02(int index, float dx, float dy) {
        if (index >= 0 && index < (int)knockVelX.size()) {
            knockVelX[index] += dx;
            knockVelY[index] += dy;
        }
    }
};

#endif // BOT02_H
