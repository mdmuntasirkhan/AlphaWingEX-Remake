#ifndef ENEMY_H
#define ENEMY_H

#include "Mesh.h"
#include "Shader.h"
#include <Matrix.h>
#include <Vector.h>
#include <vector>

using namespace MATH;

// Particle piece spawned when an enemy takes damage or is destroyed
struct Debris {
	Vec3  pos{};
	Vec3  vel{};
	Vec3  color{};
	float angle       = 0.0f;
	float spinSpeed   = 0.0f;
	float lifetime    = 0.0f;
	float maxLifetime = 0.0f;
	float pieceScale  = 0.0f;
};

// Abstract base class for all enemy types.
// Subclasses: Asteroid, Bot01, Bot02.
class Enemy {
protected:
	std::vector<Debris> debris;
	void SpawnHitDebris (const Vec3& pos, const Vec3& color, int count);
	void SpawnKillDebris(const Vec3& pos, const Vec3& color, int count);

public:
	virtual ~Enemy() = default;
	virtual void OnDestroy() = 0;
	virtual void Update(float deltaTime, float playerX = 0.0f, float playerY = 0.0f) = 0;
	virtual void Render(Shader* shader,
		const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const = 0;
	virtual void Reset() = 0;
};

#endif // ENEMY_H
