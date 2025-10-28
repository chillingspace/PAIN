// pbr.frag

#version 330 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_uniform_location : enable

#define PI 3.14159265359

in vec2 TexCoords;
layout(location = 0) out vec4 FragColor;

struct Material {
    float rough;
    float metal;
    vec3 color;
    float alwaysLit;    // bool
};

struct Light {
    vec3 position;
    vec3 L;         // light intensity
    mat4 V;         // view mtx
    mat4 P;         // perspective mtx
    float shadowMapIdx;
    float type;         // light type. 0 -> point, 1 -> dir, 2 -> spotlight(cone)
};

#define MAX_LIGHTS 16
uniform Light u_Lights[MAX_LIGHTS];
uniform float u_NumLights;
uniform vec3 u_AmbientLight;

uniform mat4 u_M;
uniform mat4 u_V;
uniform mat4 u_P;

uniform sampler2D gPos;
uniform sampler2D gCol;
uniform sampler2D gNorm;
uniform sampler2D gMaterial;

#define MAX_SHADOWMAPPED_LIGHTS 4
uniform sampler2D u_ShadowMaps[MAX_SHADOWMAPPED_LIGHTS];
uniform float u_NumShadowMaps;

Material material;


float ggxDistribution(float nDotH) {
    float alpha2 = material.rough * material.rough * material.rough * material.rough;
    float d = (nDotH * nDotH) * (alpha2 - 1.0f) + 1.0f;
    return alpha2 / (PI * d * d);
}

float geomSmith(float nDotL) {
    float k = (material.rough + 1.0f) * (material.rough + 1.0f) / 8.0f;
    float denom = nDotL * (1.0f - k) + k;
    return 1.0f / denom;
}

vec3 schlickFresnel(float lDotH) {
    vec3 f0 = vec3(0.04f); // Dielectrics
    if (material.metal == 1.0f)
        f0 = material.color;
    return f0 + (1.0f - f0) * pow(1.0f - lDotH, 5);
}

vec3 microfacetModel(vec3 position, vec3 n, Light light) {
    vec3 l;
    vec3 intensity = light.L;
    
    if (int(light.type) == 0) { 
        // point lighting
        vec3 lightPositionInView = (u_V * vec4(light.position, 1.0)).xyz;
        l = lightPositionInView - position;
        float dist = length(l);
        l = normalize(l);
        intensity *= 100 / (dist * dist);
    }
    else if (int(light.type) == 1) {
        // directional lighting
        l = normalize((u_V * vec4(light.position, 0.0)).xyz);
        // attenuation not required
    }


    vec3 diffuseBrdf = material.color;

    vec3 v = normalize(-position);
    vec3 h = normalize(v + l);
    float nDotH = dot(n, h);
    float lDotH = dot(l, h);
    float nDotL = max(dot(n, l), 0.0f);
    float nDotV = dot(n, v);
    vec3 specBrdf = 0.25f * ggxDistribution(nDotH) * schlickFresnel(lDotH) 
                            * geomSmith(nDotL) * geomSmith(nDotV);

    return (diffuseBrdf + PI * specBrdf) * intensity * nDotL;
}

float shadowIntensity(int shadow_map_idx, vec3 fragPos, vec3 normal, Light light) {
    // frag pos in light space
    vec4 fragPosLight = light.P * light.V * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLight.xyz / fragPosLight.w;    // perspective divide
    projCoords = projCoords * 0.5 + 0.5;        // convert NDC[-1, 1] to UV[0, 1]

    // check if proj coords within shadow map
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0;
    }

    float shadow_map_depth = texture(u_ShadowMaps[shadow_map_idx], projCoords.xy).r;
    float frag_depth = projCoords.z;
    
    // If shadow map is empty (cleared to 1.0), no shadows
    if (shadow_map_depth >= 0.99) {
        return 0.0; // No shadow
    }

    // bias to prevent shadow acne
    vec3 light_dir = normalize(light.position - fragPos);
    float bias = max(0.05 * (1.0 - dot(normal, light_dir)), 0.005);

    // return frag_depth - bias > shadow_map_depth ? 1.0 : 0.0;

    // PCF (Percentage Closer Filtering) for softer shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMaps[shadow_map_idx], 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(u_ShadowMaps[shadow_map_idx], projCoords.xy + vec2(x, y) * texelSize).r;
            // If shadow map is empty (no depth written), don't cast shadows
            if (pcfDepth >= 0.999) {
                shadow += 0.0;  // No shadow from empty depth
            } else {
                shadow += frag_depth - bias > pcfDepth ? 1.0 : 0.0;
            }
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

void main() {
    // vec3 vFragPosViewSpace = (u_V * vec4(vFragPos, 1.0)).xyz;
    // vec3 vNormalViewSpace = mat3(u_V) * normalize(vNormal);
    
    // vec3 color = microfacetModel(vFragPosViewSpace, vNormalViewSpace);
    // FragColor = vec4(color, 1.0);

    vec3 fragPos = texture(gPos, TexCoords).rgb;
    material.color = texture(gCol, TexCoords).rgb;
    vec3 normal = texture(gNorm, TexCoords).rgb;
    vec3 m = texture(gMaterial, TexCoords).rgb;

    material.rough = m.r;
    material.metal = m.g;
    material.alwaysLit = m.b;

    vec3 viewFragPos = (u_V * vec4(fragPos, 1.0)).xyz;
    vec3 viewNormal = mat3(u_V) * normalize(normal);

    vec3 color = vec3(0);

    if (material.alwaysLit == 0.0) {
        color = material.color * u_AmbientLight;
        for (int i=0; i < int(u_NumLights); i++) {
            vec3 light_contrib = microfacetModel(viewFragPos, viewNormal, u_Lights[i]);

            if (u_Lights[i].shadowMapIdx > -0.5) {
                // light has shadow map
                float shadow_intensity = shadowIntensity(int(u_Lights[i].shadowMapIdx), fragPos, normal, u_Lights[i]);   // in range [0,1]
                float light_intensity = 1.0 - shadow_intensity;                 // in range [0,1]
                light_contrib *= light_intensity;
            }

            color += light_contrib;
        }
    }
    else {
        color = material.color;
    }
    FragColor = vec4(color, 1.0);

    // FragColor = vec4(u_NumShadowMaps, u_NumShadowMaps, u_NumShadowMaps, 1.0);
    // FragColor = vec4(1, 0, 0, 1);

    // FragColor = vec4(material.alwaysLit, material.alwaysLit, material.alwaysLit, 1.0);
}
