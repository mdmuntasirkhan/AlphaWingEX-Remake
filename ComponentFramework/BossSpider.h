#ifndef BOSSSPIDER_H
#define BOSSSPIDER_H

#include "Enemy.h"
#include "AnimatedMesh.h"

// The spider boss.
// Right now it only stands in one place and plays the Walk animation.
// Movement, attacks and health get added step by step.
class BossSpider : public Enemy {
private:
	AnimatedMesh* creep;     // slow, used when the boss comes in
	AnimatedMesh* walk;      // normal walk
	AnimatedMesh* skitter;   // fast, used for the dash
	AnimatedMesh* current;   // the clip that is playing right now

	Vec3 pos;

	static constexpr float kScale = 0.7f;   // render scale of the spider mesh

public:
	BossSpider();
	~BossSpider();

	bool OnCreate();
	void OnDestroy() override;
	void Update(float deltaTime, float playerX = 0.0f, float playerY = 0.0f) override;
	void Render(Shader* shader,
		const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const override;
	void Reset() override;
};

#endif // BOSSSPIDER_H