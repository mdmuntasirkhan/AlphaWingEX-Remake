#ifndef MESH_H
#define MESH_H

#include <glew.h>
#include <vector>
#include <Vector.h>

using namespace MATH;

// Loads an OBJ file and uploads vertex, normal, and UV data to the GPU.
// Cannot be copied or moved
class Mesh {
private:
    const char* filename;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvCoords;
    size_t            dateLength; // number of vertices stored on GPU
    GLenum            drawmode;
    GLuint            vao, vbo;

    void LoadModel(const char* filename);       // parses OBJ via tinyobjloader
    void StoreMeshData(GLenum drawmode_);       // uploads data to GPU, clears CPU vectors

    // Non-copyable, non-movable
    Mesh(const Mesh&) = delete;
    Mesh(Mesh&&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh& operator=(Mesh&&) = delete;

public:
    Mesh(const char* filename_);
    ~Mesh();

    bool OnCreate();                        // loads OBJ and uploads to GPU
    void OnDestroy();                       // frees VAO and VBO
    void Update(const float deltaTime);     // unused, reserved for future animation
    void Render() const;                    // draws with the stored draw mode
    void Render(GLenum drawmode_) const;    // draws with a custom draw mode
};

#endif // MESH_H
