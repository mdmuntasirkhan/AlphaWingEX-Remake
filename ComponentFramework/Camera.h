#ifndef CAMERA_H
#define CAMERA_H

#include <vector.h>
#include <matrix.h>
#include <Quaternion.h>

using namespace MATH;

class Camera {
private:
	Vec3 camPos;
	Matrix4 viewMatrix;
	Matrix4 projectionMatrix;
	Quaternion orientation;

public:
	Camera();
	~Camera();

	bool OnCreate();
	void OnDestroy();
	void Render() const;

	// Getters
	Vec3 getCamPos() const { return camPos; }
	Matrix4 getViewMatrix() const { return viewMatrix; }
	Matrix4 getProjectionMatrix() const { return projectionMatrix; }
	Quaternion getCamOrientation() const { return orientation; }

	// Returns the direction the camera is currently facing
	Vec3 getForwardVec() const { return orientation * Vec3(0.0f, 0.0f, 1.0f); }
};

#endif // CAMERA_H
