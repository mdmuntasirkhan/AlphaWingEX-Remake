#ifndef BODY_H
#define BODY_H
#include <Vector.h> /// This is in GameDev
#include <Quaternion.h>
using namespace MATH; 

/// Just forward declair these classes so I can define a pointer to them
/// Used later in the course
class Mesh;
class Texture;
class Shader;

class Body {
public:
    Body();
    ~Body();
private: /// Physics stuff
	Vec3 pos;
	Vec3 vel;
	Vec3 accel;
	float mass;

	// rotation stuff
	Quaternion orientation;
	Matrix4 modelMatrix;

	float radius; // for size / (collition?)

	// check for types of collision
	// 1 - sphere
	// 2 - rectangular (plane)
	int collisionType;

	friend class Collision;
private:
	// Graphics stuff 
	Mesh *mesh;
	Texture *texture;

public:
	bool OnCreate();
	void OnDestroy();
	void Update(float deltaTime);
	void Render(Shader *shader) const;
	void ApplyForce(Vec3 force);
	void setAccel(const Vec3 &accel_) { accel = accel_;}
	void setVel(const Vec3& vel_) { vel = vel_; }

	void setMesh(const char* filename);
};

#endif
