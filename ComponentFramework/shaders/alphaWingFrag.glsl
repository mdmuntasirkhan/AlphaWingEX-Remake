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
// Three crescent glint centers, in shield mesh local space - each traces the
// front curve of the shield top to bottom, spaced 1/3 of a cycle apart.
uniform vec3  shieldGlowPointA;
uniform vec3  shieldGlowPointB;
uniform vec3  shieldGlowPointC;
uniform float shieldGlowRadius; // falloff radius of each crescent, in local-space units

// Brightness contribution of one crescent at this fragment. Distance is
// weighted anisotropically - squashed in Y, stretched in X - so each glint
// reads as a thin curved crescent sliver hugging the dome, not a round blob.
// (Z is left unweighted: since glowPoint already sits on the dome's front
// curve, fragments near the silhouette naturally differ more in Z, which is
// exactly what tapers the crescent off as it nears the rim.)
float ShieldCrescent(vec3 fragLocalPos, vec3 glowPoint, float radius) {
    vec3 d = fragLocalPos - glowPoint;
    vec3 weighted = vec3(d.x * 0.4, d.y * 2.5, d.z);
    float dist = length(weighted);
    // smoothstep requires edge0 < edge1, so go 0->1 over the radius then
    // invert it to get a falloff that's brightest at dist == 0.
    return 1.0 - smoothstep(0.0, radius, dist);
}

void main() {
    if (emissive > 0.5) {
        // Flat color (shield glow) - the dome stays visible the whole time,
        // brightened by 3 crescent-shaped glints sliding down its front
        // curve. Using local-space distance (not a flat Y cut) means each
        // glint's edge is always a curve, never a straight chord, so it
        // hugs the dome's shape like a real reflection.
        float glintA = ShieldCrescent(fragLocalPos, shieldGlowPointA, shieldGlowRadius);
        float glintB = ShieldCrescent(fragLocalPos, shieldGlowPointB, shieldGlowRadius);
        float glintC = ShieldCrescent(fragLocalPos, shieldGlowPointC, shieldGlowRadius);
        float glint = max(glintA, max(glintB, glintC));
        fragColor = vec4(color.rgb + glint * vec3(0.9, 0.95, 1.0), color.a + glint * 0.6);
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