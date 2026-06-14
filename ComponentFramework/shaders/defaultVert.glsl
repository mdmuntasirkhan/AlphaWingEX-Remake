#version 450
#extension GL_ARB_separate_shader_objects : enable

// attribute
layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUVCoord;

// uniform
layout(location = 0) uniform mat4 projectionMatrix; // could use on FOV // For field of view
layout(location = 1) uniform mat4 viewMatrix; // For cameras (point of view)
layout(location = 2) uniform mat4 modelMatrix; // one for each body (actor)

void main() {
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(inVertex, 1.0);
}