#version 450
#extension GL_ARB_separate_shader_objects : enable

// Coming IN from the vertex shader
layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragPos;

// Going OUT to screen
layout(location = 0) out vec4 fragColor;

// You send these from C++
layout(location = 3) uniform vec4 color;       // object color
layout(location = 4) uniform vec3 lightPos;    // where the light is
layout(location = 5) uniform vec3 viewPos;     // where the camera is

void main() {
    vec3 normal    = normalize(fragNormal);
    vec3 lightDir  = normalize(lightPos - fragPos);
    vec3 viewDir   = normalize(viewPos - fragPos);

    // Ambient — base brightness so nothing is pure black
    vec3 ambient = 0.3 * color.rgb;

    // Diffuse — bright where light hits
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * color.rgb;

    // Specular — shiny highlight
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = 0.4 * spec * vec3(1.0, 1.0, 1.0);

    vec3 result = ambient + diffuse + specular;
    fragColor = vec4(result, color.a);
}