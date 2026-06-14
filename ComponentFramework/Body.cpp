#include "Body.h"
#include <QMath.h>
#include <MMath.h>
#include "Mesh.h"
#include "Texture.h"
#include "Shader.h"

Body::Body(): pos{}, vel{}, accel{}, mass{1.0f}, radius(1.0f), 
mesh{nullptr}, texture{nullptr}{
	// set inertia
}

Body::~Body() {}

void Body::Update(float deltaTime) {
	/// From 1st semester physics class
	pos += vel * deltaTime + 0.5f * accel * deltaTime * deltaTime;
	vel += accel * deltaTime;
}

void Body::ApplyForce(Vec3 force) {
	accel = force / mass;
}

bool Body::OnCreate() {
	return true;
}

void Body::setMesh(const char* filename) {
	mesh = new Mesh(filename);
	mesh->OnCreate();
	modelMatrix.loadIdentity();
}

void Body::OnDestroy() {
	if (mesh) {
		mesh->OnDestroy();
		delete mesh;
	}

	if (texture) {
		delete texture;
	}
}

void Body::Render(Shader* shader) const {
	glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, modelMatrix);
	mesh->Render(GL_TRIANGLES);
}

