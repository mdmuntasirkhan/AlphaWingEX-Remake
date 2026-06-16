#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 fragLocalPos;
layout(location = 0) out vec4 fragColor;

uniform vec4  color;
uniform vec3  lightPos;
uniform vec3  viewPos;
uniform float emissive;

void main() {
    if (emissive > 0.5) {
        // Fresnel rim: transparent at center, opaque at edge — classic shield bubble.
        // Back faces are culled by the caller so only the outer surface renders.
        vec3  n      = normalize(fragNormal);
        vec3  vDir   = normalize(viewPos - fragPos);
        float rim    = 1.0 - max(dot(n, vDir), 0.0);
        fragColor    = vec4(color.rgb, pow(rim, 1.5) * 0.8);
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