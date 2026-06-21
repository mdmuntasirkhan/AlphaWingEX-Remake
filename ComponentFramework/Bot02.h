#pragma once
#ifndef BOT02_H
#define BOT02_H

#include "Enemy.h"
#include <vector>

class Bot02 : public Enemy {
private:
    Mesh* bodyMesh;
    Mesh* cockpitMesh;
    Mesh* finMesh;
    Mesh* thrustMesh;
    Mesh* fragmentMesh;
    Mesh* bulletMesh;
    float thrustTimer;
    float hoverTimer;

    // Per-instance data (always 0 or 2 active)
    std::vector<Vec3>  positions;
    std::vector<Vec3>  targetPositions;
    std::vector<float> hoverPhases;
    std::vector<int>   hp;
    std::vector<float> hitTimers;
    std::vector<float> fireTimers;   // per-instance fire cooldown

    // Projectile pool
    std::vector<Vec3> bulletPositions;
    std::vector<Vec3> bulletVelocities;

    static constexpr float kHoverX         = 6.0f;
    static constexpr float kHoverYOffset   = 3.0f;
    static constexpr float kApproachSpeed  = 3.0f;
    static constexpr float kHoverAmplitude = 0.15f;
    static constexpr float kHoverFrequency = 1.8f;
    static constexpr int   kHP             = 20;
    static constexpr float kFireInterval   = 3.0f;  // seconds between shots per Bot02
    static constexpr float kBulletSpeed    = 3.0f;  // slow but precise
    static constexpr float kBulletScale    = 0.08f;

public:
    Bot02();
    ~Bot02();

    bool OnCreate(const char* bodyFile, const char* cockpitFile,
                  const char* finFile,  const char* thrustFile,
                  const char* fragmentFile, const char* bulletFile);
    void OnDestroy() override;
    void Update(float deltaTime, float playerX = 0.0f, float playerY = 0.0f) override;
    void Render(Shader* shader,
        const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const override;
    void Reset() override;

    void Spawn(float playerY);

    std::vector<Vec3>& GetPositions()       { return positions; }
    std::vector<Vec3>& GetBulletPositions() { return bulletPositions; }
    std::vector<Vec3>& GetBulletVelocities(){ return bulletVelocities; }
    int GetCount() const { return (int)positions.size(); }

    bool DamageBot02(int index, int amount = 1);
    void RemoveBot02(int index);
    void RemoveBullet(int index);
};

#endif // BOT02_H
