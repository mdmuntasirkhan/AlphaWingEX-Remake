#ifndef BODY_H
#define BODY_H

#include <Vector.h>
#include <Quaternion.h>

using namespace MATH; 

class Mesh;
class Texture;
class Shader;

class Body {
private:
	// Physics
	Vec3 pos;
	Vec3 vel;
	Vec3 accel;
	float mass;
	float radius;
	Quaternion orientation;
	Matrix4 modelMatrix;

	// Collision Type
	int collisionType;

	// Graphics
	Mesh* mesh;
	Texture* texture;

	friend class Collision;
public:
    Body();
    ~Body();

	bool OnCreate();
	void OnDestroy();
	void Update(float deltaTime);
	void Render(Shader *shader) const;
	void ApplyForce(Vec3 force);

	void setAccel(const Vec3 &accel_) { accel = accel_;}
	void setVel(const Vec3& vel_) { vel = vel_; }
	void setMesh(const char* filename);
};

#endif // BODY_H
