#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUVCoord;

layout(location = 0) uniform mat4 projectionMatrix;
layout(location = 1) uniform mat4 viewMatrix;
layout(location = 2) uniform mat4 modelMatrix;

// Passing to frag shader
layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragPos;

void main() {
    vec4 worldPos  = modelMatrix * vec4(inVertex, 1.0);
    fragPos        = worldPos.xyz;
    fragNormal     = mat3(transpose(inverse(modelMatrix))) * inNormal;
    gl_Position    = projectionMatrix * viewMatrix * worldPos;
}