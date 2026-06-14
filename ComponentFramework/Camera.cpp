#include "Camera.h"
#include <MMath.h>
#include <QMath.h>

Camera::Camera() : camPos(Vec3()), projectionMatrix(Matrix4()), 
viewMatrix(Matrix4()), orientation(Quaternion()){
	
}

Camera::~Camera() { /* Clear Memory */ }

bool Camera::OnCreate() {
	projectionMatrix = MMath::perspective(45.0f, 16.0f / 9.0f, 0.5f, 100.0f);
	orientation = QMath::angleAxisRotation(0.0f, Vec3(0.0f, 1.0f, 0.0f));
	camPos = Vec3(0.0f, 0.0f, 10.0f);

	Matrix4 T = MMath::translate(camPos);
	Matrix4 R = MMath::toMatrix4(orientation);

	viewMatrix = MMath::inverse(R) * MMath::inverse(T);

	return true;
}

void Camera::OnDestroy() {
	
}

