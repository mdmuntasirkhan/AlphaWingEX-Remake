#ifndef ANIMATEDMESH_H
#define ANIMATEDMESH_H

#include <string>
#include <vector>
#include "Mesh.h"

// Plays a numbered set of OBJ files one after another, like a flipbook.
// For example the prefix "meshes/Spider/Walk" loads Walk_00.obj, Walk_01.obj and so on.
// Cannot be copied or moved (it owns Mesh pointers)
class AnimatedMesh {
private:
	std::string prefix;          // start of the file names
	int frameCount;              // how many OBJ files there are
	float fps;                   // how many frames play per second at normal speed
	float playSpeed;             // 1 is normal, 2 is twice as fast, negative plays backward
	float currentFrame;          // float so slow speeds still move a little each update
	std::vector<Mesh*> frames;

	AnimatedMesh(const AnimatedMesh&) = delete;
	AnimatedMesh(AnimatedMesh&&) = delete;
	AnimatedMesh& operator=(const AnimatedMesh&) = delete;
	AnimatedMesh& operator=(AnimatedMesh&&) = delete;

public:
	AnimatedMesh(const std::string& prefix_, int frameCount_, float fps_ = 24.0f);
	~AnimatedMesh();

	bool OnCreate();                      // loads every frame and uploads it to the GPU
	void OnDestroy();                     // frees every frame
	void Update(const float deltaTime);   // moves to the next frame when enough time passed
	void Render() const;                  // draws the current frame

	void SetPlaySpeed(float speed_) { playSpeed = speed_; }
	void Restart() { currentFrame = 0.0f; }
	int GetFrameCount() const { return frameCount; }
	int GetCurrentFrame() const { return static_cast<int>(currentFrame); }
};

#endif // ANIMATEDMESH_H