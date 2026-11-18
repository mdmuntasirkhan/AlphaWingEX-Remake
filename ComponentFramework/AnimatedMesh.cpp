#include "AnimatedMesh.h"
#include <cstdio>

AnimatedMesh::AnimatedMesh(const std::string& prefix_, int frameCount_, float fps_) :
	prefix{ prefix_ }, frameCount{ frameCount_ }, fps{ fps_ },
	playSpeed{ 1.0f }, currentFrame{ 0.0f } {
}

AnimatedMesh::~AnimatedMesh() {
	OnDestroy();
}

bool AnimatedMesh::OnCreate() {
	for (int i = 0; i < frameCount; i++) {
		// builds a name like meshes/Spider/Walk_07.obj
		char fileName[256];
		snprintf(fileName, sizeof(fileName), "%s_%02d.obj", prefix.c_str(), i);

		// Mesh only keeps the pointer, but OnCreate reads the file right away so this is safe
		Mesh* frame = new Mesh(fileName);
		if (frame->OnCreate() == false) {
			delete frame;
			return false;
		}
		frames.push_back(frame);
	}
	return true;
}

void AnimatedMesh::OnDestroy() {
	for (Mesh* frame : frames) {
		frame->OnDestroy();
		delete frame;
	}
	frames.clear();
}

void AnimatedMesh::Update(const float deltaTime) {
	if (frames.empty()) return;

	currentFrame += deltaTime * fps * playSpeed;

	// go back to the start when we pass the last frame, so the animation loops
	while (currentFrame >= static_cast<float>(frameCount)) {
		currentFrame -= static_cast<float>(frameCount);
	}
}

void AnimatedMesh::Render() const {
	if (frames.empty()) return;
	frames[static_cast<int>(currentFrame)]->Render();
}