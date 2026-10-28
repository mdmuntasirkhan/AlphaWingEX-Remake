#ifndef SCENEJA_H
#define SCENEJA_H

#include "Scene.h"
#include "Vector.h"
#include <Matrix.h>
#include "SoundManager.h"
#include <memory>
#include <string_view>

#include "FMOD_AudioManager.h"

using namespace MATH;

/// Forward declarations 
union SDL_Event;
class Body;
class Mesh;
class Shader;
class Texture;
class Camera;

const std::string_view BGM_NAME = "Main BGM";

class SceneJA : public Scene {
private:
	Body* testOBJ;
	Shader* shader;
	Mesh* mesh;
	Matrix4 projectionMatrix;
	Matrix4 viewMatrix;
	Matrix4 earthModelMatrix, moonModelMatrix, marioModelMatrix;
	bool drawInWireMode;

	Vec3 lightPos;

	Camera* camera;

	SoundManager* soundManager;
	Sound* sound1, *sound2;

	std::unique_ptr<FMOD_AudioManager> FMOD_manager;

public:
	explicit SceneJA();
	virtual ~SceneJA();

	virtual bool OnCreate() override;
	virtual void OnDestroy() override;
	virtual void Update(const float deltaTime) override;
	virtual void Render() const override;
	virtual void HandleEvents(const SDL_Event& sdlEvent) override;
};


#endif // SCENEJA_H
