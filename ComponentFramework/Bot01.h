#pragma once
#ifndef BOT01_H
#define BOT01_H

#include "Enemy.h"
#include <vector>

class Bot01 : public Enemy {
private:
	Mesh* bot01Mesh;
	Mesh* bot01ThrustMesh;
	Mesh* fragmentMesh;		// asteroid mesh shape reused for debris fragments
	float thrustTimer;

	std::vector<Vec3>  bot01Positions;
	std::vector<float> bot01YVelocities;
	std::vector<float> bot01XKnockbackVels; // separate from march speed — decays with drag
	std::vector<int>   bot01HP;
	std::vector<float> bot01HitTimers;

	float bot01SteerForce;
	float bot01YDamping;
	float bot01YMaxSpeed;
	float bot01Speed;
	float bot01SpawnTimer;
	float bot01SpawnInterval;
	float totalTime;			// wave-progression timer (save/load)

	int waveSize;				// bots to spawn per wave
	int waveSpawned;			// how many have been spawned this wave

public:
	Bot01();
	~Bot01();

	bool OnCreate(const char* meshFile, const char* fragmentMeshFile);
	void OnDestroy() override;
	void Update(float deltaTime, float playerX = 0.0f, float playerY = 0.0f) override;
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
	void ResetWave()             { waveSpawned = 0; bot01SpawnTimer = 0.0f; }

	bool DamageBot01(int index, int amount = 1);
	void RemoveBot01(int index);
	// Knockback — kept separate from AI movement so they don't fight each other
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
