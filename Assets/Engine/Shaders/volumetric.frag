// volumetric.frag
// Screen-space volumetric light scattering (god rays)
// Ray marches from camera to each fragment, accumulating in-scattered light.

#version 330 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_uniform_location : enable

in vec2 TexCoords;
layout(location = 0) out vec4 FragColor;

// G-buffer world-space positions
uniform sampler2D gPos;

struct Light {
    vec3 position;
    vec3 L;
    mat4 V;
    mat4 P;
    float shadowMapIdx;
    float type;         // 0=point, 1=directional, 2=spotlight
    vec3 direction;
    float innerCutoff;  // cosine of inner angle
    float outerCutoff;  // cosine of outer angle
};

#define MAX_LIGHTS 16
uniform Light u_Lights[MAX_LIGHTS];
uniform float u_NumLights;

#define MAX_SHADOWMAPPED_LIGHTS 4
uniform sampler2D u_ShadowMaps[MAX_SHADOWMAPPED_LIGHTS];

uniform vec3  u_CamPos;
uniform mat4  u_InvVP;          // inverse(projection * view)

uniform float u_VolumetricIntensity;  // overall brightness multiplier
uniform float u_VolumetricSteps;      // number of ray march steps
uniform float u_VolumetricMaxDist;    // maximum ray length
uniform float u_VolumetricScatter;    // Mie g factor (0=isotropic, 0.9=heavy forward)

// Henyey-Greenstein Mie scattering phase function
float hgPhase(float cosTheta, float g) {
    float g2 = g * g;
    float denom = pow(max(1.0 + g2 - 2.0 * g * cosTheta, 0.0001), 1.5);
    return (1.0 - g2) / (4.0 * 3.14159265 * denom);
}

// Returns 1.0 if worldPos is in shadow, 0.0 if lit
float sampleShadow(int shadowIdx, vec3 worldPos, Light light) {
    vec4 fragPosLight = light.P * light.V * vec4(worldPos, 1.0);
    vec3 projCoords   = fragPosLight.xyz / fragPosLight.w;
    projCoords        = projCoords * 0.5 + 0.5;

    // Outside shadow map bounds = lit
    if (projCoords.z > 1.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float shadowDepth = texture(u_ShadowMaps[shadowIdx], projCoords.xy).r;
    // Empty shadow map (cleared to 1.0) means no caster = lit
    if (shadowDepth >= 0.99) return 0.0;

    return projCoords.z - 0.005 > shadowDepth ? 1.0 : 0.0;
}

void main() {
    vec3 fragWorldPos = texture(gPos, TexCoords).rgb;

    // Reconstruct world-space ray direction from UV
    vec2 ndc        = TexCoords * 2.0 - 1.0;
    vec4 worldFar   = u_InvVP * vec4(ndc, 1.0, 1.0);
    worldFar       /= worldFar.w;
    vec3 rayDir     = normalize(worldFar.xyz - u_CamPos);

    // Decide ray length: stop at geometry if present, else use max distance
    // Sky/background pixels have gPos == vec3(0) after the G-buffer clear
    bool hasSurface = dot(fragWorldPos, fragWorldPos) > 0.001;
    float distToSurface = hasSurface ? length(fragWorldPos - u_CamPos) : u_VolumetricMaxDist;
    float rayLen    = min(distToSurface, u_VolumetricMaxDist);

    int   numSteps  = int(u_VolumetricSteps);
    float stepSize  = rayLen / float(numSteps);

    vec3 accumulated = vec3(0.0);

    for (int s = 0; s < numSteps; s++) {
        float t         = (float(s) + 0.5) * stepSize;
        vec3  samplePos = u_CamPos + rayDir * t;

        for (int i = 0; i < int(u_NumLights); i++) {
            Light light = u_Lights[i];

            // Without a shadow map there is no occlusion data — the light would
            // brighten the entire volume uniformly with no visible shafts, so skip it.
            if (light.shadowMapIdx < -0.5) continue;

            vec3  lightContrib = vec3(0.0);

            if (int(light.type) == 0) {
                // ----- Point light -----
                vec3  toLight     = light.position - samplePos;
                float dist        = length(toLight);
                float attenuation = 100.0 / max(dist * dist, 0.001);

                float inShadow    = sampleShadow(int(light.shadowMapIdx), samplePos, light);
                float cosTheta    = dot(normalize(toLight), -rayDir);
                float phase       = hgPhase(cosTheta, u_VolumetricScatter);

                lightContrib = light.L * attenuation * phase * (1.0 - inShadow);
            }
            else if (int(light.type) == 1) {
                // ----- Directional light -----
                vec3 lightDir  = normalize(-light.direction);
                float inShadow = sampleShadow(int(light.shadowMapIdx), samplePos, light);
                float cosTheta = dot(lightDir, -rayDir);
                float phase    = hgPhase(cosTheta, u_VolumetricScatter);

                lightContrib = light.L * phase * (1.0 - inShadow);
            }
            else if (int(light.type) == 2) {
                // ----- Spotlight -----
                vec3  toLight     = light.position - samplePos;
                float dist        = length(toLight);
                float attenuation = 100.0 / max(dist * dist, 0.001);

                vec3  toLightNorm  = normalize(toLight);
                vec3  spotDir      = normalize(light.direction);
                float theta        = dot(-toLightNorm, spotDir);
                float epsilon      = max(light.innerCutoff - light.outerCutoff, 0.001);
                float spotIntensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);

                float inShadow = sampleShadow(int(light.shadowMapIdx), samplePos, light);
                float cosTheta = dot(toLightNorm, -rayDir);
                float phase    = hgPhase(cosTheta, u_VolumetricScatter);

                lightContrib = light.L * attenuation * spotIntensity * phase * (1.0 - inShadow);
            }

            accumulated += lightContrib * stepSize;
        }
    }

    accumulated *= u_VolumetricIntensity;
    FragColor = vec4(accumulated, 1.0);
}
