#include "BossSpider.h"
#include "GameConstants.h"
#include <MMath.h>

BossSpider::BossSpider() :
	creep{ nullptr }, walk{ nullptr }, skitter{ nullptr }, current{ nullptr },
	pos{ 7.0f, -0.3f, GameConst::kWorldZ } {
}

BossSpider::~BossSpider() {}

bool BossSpider::OnCreate() {
	// frame counts come from the baked files (Creep_00 to Creep_70 is 71 frames and so on)
	creep = new AnimatedMesh("meshes/Spider/Creep", 71);
	walk = new AnimatedMesh("meshes/Spider/Walk", 47);
	skitter = new AnimatedMesh("meshes/Spider/Skitter", 35);

	if (creep->OnCreate() == false || walk->OnCreate() == false || skitter->OnCreate() == false) {
		return false;
	}

	current = walk;
	return true;
}

void BossSpider::OnDestroy() {
	delete creep;
	delete walk;
	delete skitter;
	creep = walk = skitter = nullptr;
	current = nullptr;
}

void BossSpider::Update(float deltaTime, float playerX, float playerY) {
	current->Update(deltaTime);
}

void BossSpider::Render(Shader* shader,
	const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const {
	glUniformMatrix4fv(shader->GetUniformID("projectionMatrix"), 1, GL_FALSE, projectionMatrix);
	glUniformMatrix4fv(shader->GetUniformID("viewMatrix"), 1, GL_FALSE, viewMatrix);

	// white, so the colors baked into the mesh show up as they are
	glUniform4f(shader->GetUniformID("color"), 1.0f, 1.0f, 1.0f, 1.0f);
	glUniform1f(shader->GetUniformID("flash"), 0.0f);

	Matrix4 m = MMath::translate(pos) * MMath::scale(kScale, kScale, kScale);
	glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
	current->Render();
}

void BossSpider::Reset() {
	pos = Vec3(7.0f, -0.3f, GameConst::kWorldZ);
	current = walk;
	walk->Restart();
}