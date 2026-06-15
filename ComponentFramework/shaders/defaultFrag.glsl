#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragPos;
layout(location = 0) out vec4 fragColor;

uniform vec4  color;
uniform vec3  lightPos;
uniform vec3  viewPos;
uniform float emissive;

void main() {
    if (emissive > 0.5) {
        // Flat color, no lighting - perfect for shield glow
        fragColor = color;
        return;
    }

    // Normal Phong lighting (your existing code)
    vec3 normal    = normalize(fragNormal);
    vec3 lightDir  = normalize(lightPos - fragPos);
    vec3 viewDir   = normalize(viewPos - fragPos);

    vec3 ambient   = 0.3 * color.rgb;
    float diff     = max(dot(normal, lightDir), 0.0);
    vec3 diffuse   = diff * color.rgb;
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec     = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular  = 0.4 * spec * vec3(1.0);

    fragColor = vec4(ambient + diffuse + specular, color.a);
}