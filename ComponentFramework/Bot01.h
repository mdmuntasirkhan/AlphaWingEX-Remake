#pragma once
#ifndef BOT01_H
#define BOT01_H

#include "Enemy.h"
#include <vector>

// Industry-standard wave descriptor — one entry per wave in the cycle table
enum class Bot01WaveType { STANDARD, PINCER, SHIELDED };

struct Bot01WaveDesc {
    Bot01WaveType type;
    int           count;
    float         interval;   // seconds between standard spawns (0 = all spawn at once)
};

class Bot01 : public Enemy {
private:
    Mesh* bot01Mesh;
    Mesh* bot01ThrustMesh;
    Mesh* fragmentMesh;     // asteroid mesh shape reused for debris fragments
    Mesh* shieldMesh;       // same OBJ as player shield, rendered orange on shielded bots
    float thrustTimer;

    std::vector<Vec3>  bot01Positions;
    std::vector<float> bot01XVelocities;
    std::vector<float> bot01YVelocities;
    std::vector<float> bot01XKnockbackVels;
    std::vector<int>   bot01HP;
    std::vector<float> bot01HitTimers;

    // Per-instance shield state — present for every bot; only bots with hasShield==true use them
    std::vector<bool>  bot01HasShield;
    std::vector<bool>  bot01ShieldActive;
    std::vector<bool>  bot01ShieldOnCooldown;
    std::vector<float> bot01ShieldTimer;
    std::vector<float> bot01ShieldCooldownTimer;

    static constexpr float kShieldDuration  = 5.0f;   // shield stays up for 5 s
    static constexpr float kShieldCooldown  = 10.0f;  // 10 s recharge (2x player's 5 s)
    static constexpr float kShieldProximity = 3.0f;   // activate when missile within 3 world units

    float bot01SteerForce;
    float bot01YDamping;
    float bot01YMaxSpeed;
    float bot01Speed;
    float bot01SpawnTimer;
    float bot01SpawnInterval;
    float totalTime;
    bool  spawningEnabled;

    int waveSize;
    int waveSpawned;
    int currentWaveIndex;
    Bot01WaveType currentWaveType;

    // Internal helper — pushes one bot to all parallel vectors
    void SpawnBot01(float x, float y, bool hasShield);

public:
    Bot01();
    ~Bot01();

    bool OnCreate(const char* meshFile, const char* fragmentMeshFile);
    void OnDestroy() override;
    void Update(float deltaTime, float playerX = 0.0f, float playerY = 0.0f) override;
    // Called from SceneMuntasir after Update() so shielded bots react to incoming missiles
    void UpdateShields(float deltaTime, const std::vector<Vec3>& missilePositions);
    void Render(Shader* shader,
        const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const override;
    void Reset() override;

    std::vector<Vec3>& GetBot01Positions() { return bot01Positions; }
    int   GetCount()    const { return (int)bot01Positions.size(); }
    float GetBot01Speed() const { return bot01Speed; }
    float GetTotalTime()  const { return totalTime; }
    void  SetTotalTime(float t)  { totalTime = t; }

    // True once all waveSize bots have spawned AND all are gone (killed/despawned)
    bool IsWaveComplete() const { return waveSpawned >= waveSize && bot01Positions.empty(); }
    void ResetWave();   // advances to next wave in the cycle table
    void SetSpawningEnabled(bool enabled) { spawningEnabled = enabled; }

    // Start a wave on demand — called by LevelDirector when a SPAWN_BOT01_* event fires.
    // Replaces the internal auto-cycle: the level script is now the sole authority on timing.
    void TriggerWave(Bot01WaveType type, int count, float interval);

    bool IsBot01ShieldActive(int i) const {
        return i >= 0 && i < (int)bot01ShieldActive.size() && bot01ShieldActive[i];
    }

    bool DamageBot01(int index, int amount = 1);
    void RemoveBot01(int index);
    void PushX(int index, float impulse) {
        if (index >= 0 && index < (int)bot01XKnockbackVels.size())
            bot01XKnockbackVels[index] += impulse;
    }
    void PushY(int index, float impulse) {
        if (index >= 0 && index < (int)bot01YVelocities.size())
            bot01YVelocities[index] += impulse;
    }
};

#endif // BOT01_H
