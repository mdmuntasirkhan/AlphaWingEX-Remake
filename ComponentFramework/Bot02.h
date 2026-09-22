// AlphaWingEX-Remake
// Author: Muntasir
// Brief:  Dual hovering gunship pair — always spawns as two units.

#ifndef BOT02_H
#define BOT02_H

#include "Enemy.h"
#include <vector>

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

    std::vector<Vec3>  positions;
    std::vector<Vec3>  targetPositions; // fixed hover point each instance glides toward
    std::vector<float> hoverPhases;     // offset so top and bottom oscillate out of syn
    std::vector<int>   hp;
    std::vector<float> hitTimers;
    std::vector<float> fireTimers;
    std::vector<float> knockVelX;
    std::vector<float> knockVelY;

    // Bullet pool
    std::vector<Vec3>  bulletPositions;
    std::vector<Vec3>  bulletVelocities;
    std::vector<bool>  bulletReflected;

    // Tuning constants
    static constexpr float kScale           = 0.17f;
    static constexpr float kKnockbackDecay  = 9.0f;
    static constexpr float kHoverX         = 6.0f;
    static constexpr float kHoverYOffset   = 3.0f;      // top at +Y, bottom at -Y
    static constexpr float kApproachSpeed  = 3.0f;
    static constexpr float kHoverAmplitude = 0.15f;
    static constexpr float kHoverFrequency = 1.8f;
    static constexpr int   kHP             = 20;
    static constexpr float kFireInterval   = 3.0f;
    static constexpr float kBulletSpeed    = 3.0f;
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

    // Spawn. Always creates the top + bottom pair together
    void Spawn(float playerY);

    // Getters for collision check in sceneMuntasir
    std::vector<Vec3>& GetPositions()       { return positions; }
    std::vector<Vec3>& GetBulletPositions() { return bulletPositions; }
    std::vector<Vec3>& GetBulletVelocities(){ return bulletVelocities; }
    int GetCount() const { return (int)positions.size(); }

    bool DamageBot02(int index, int amount = 1);        // returns true if destroyed
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
