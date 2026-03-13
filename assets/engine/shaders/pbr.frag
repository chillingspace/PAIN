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
    float debugging_geometry;    // bool
};

struct Light {
    vec3 position;
    vec3 L;         // light intensity
    mat4 V;         // view mtx
    mat4 P;         // perspective mtx
    float shadowMapIdx;
    float type;         // light type. 0 -> point, 1 -> dir, 2 -> spotlight(cone)

    // spotlight
    vec3 direction;     // Direction the spotlight is facing
    float innerCutoff;  // Cosine of inner angle
    float outerCutoff;  // Cosine of outer angle
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
uniform sampler2D gEmission;

// for ibl
uniform float u_UseIbl;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLut;
uniform vec3 u_CamPos;

#define MAX_SHADOWMAPPED_LIGHTS 4
uniform sampler2D u_ShadowMaps[MAX_SHADOWMAPPED_LIGHTS];
uniform float u_NumShadowMaps;

Material material;

// debug
uniform float DEBUG_TYPE;


float ggxDistribution(float nDotH) {
    float alpha2 = material.rough * material.rough * material.rough * material.rough;
    float d = (nDotH * nDotH) * (alpha2 - 1.0f) + 1.0f;
    return alpha2 / (PI * d * d + 0.0001f);
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
    return f0 + (1.0f - f0) * pow(clamp(1.0f - lDotH, 0.0, 1.0), 5.0);
}

// for ibl. schlickFresnel but with roughness
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
} 

float SampleShadowMap(int shadow_map_idx, vec2 uv) {
    return texture(u_ShadowMaps[shadow_map_idx], uv).r;
}

vec2 ShadowTexelSize(int shadow_map_idx) {
    return 1.0 / vec2(textureSize(u_ShadowMaps[shadow_map_idx], 0));
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
        intensity *= 100.0 / max(dist * dist, 0.001);
    }
    else if (int(light.type) == 1) {
        // directional lighting
        vec3 lightDirView = mat3(u_V) * normalize(-light.direction);
        l = normalize(lightDirView);

        // attenuation not required
    }
    else if (int(light.type) == 2) {
        // spotlight lighting
        
        vec3 lightPositionInView = (u_V * vec4(light.position, 1.0)).xyz;
        l = lightPositionInView - position;
        float dist = length(l);
        l = normalize(l); // Vector FROM Surface TO Light
        
        // distance attenuation
        intensity *= 100.0 / max(dist * dist, 0.001);

        // cone attenuation
        // have to convert world space to view space
        vec3 spotDirView = normalize(mat3(u_V) * light.direction); 
        
        // calculate angle
        float theta = dot(-l, spotDirView); 
        
        // clamping to kill the light
        float epsilon = max(light.innerCutoff - light.outerCutoff, 0.001);
        float spotIntensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
        
        intensity *= spotIntensity;
    }

    vec3 diffuseBrdf = material.color;

    vec3 v = normalize(-position);
    vec3 h = normalize(v + l);
    float nDotH = max(dot(n, h), 0.0);
    float lDotH = max(dot(l, h), 0.0);
    float nDotL = max(dot(n, l), 0.0);
    float nDotV = max(dot(n, v), 0.0);
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

    float shadow_map_depth = SampleShadowMap(shadow_map_idx, projCoords.xy);
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
    vec2 texelSize = ShadowTexelSize(shadow_map_idx);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = SampleShadowMap(shadow_map_idx, projCoords.xy + vec2(x, y) * texelSize);
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

const int IBL_DEBUG_TYPE_BASE = 8;
const bool IBL_FLIP_Y_AXIS = true;

void main() {
    if (u_NumShadowMaps > MAX_SHADOWMAPPED_LIGHTS) {
        FragColor = vec4(1, 0, 1, 1);
        return;
    }


    vec3 fragPos = texture(gPos, TexCoords).rgb;
    material.color = texture(gCol, TexCoords).rgb;
    vec3 normal = texture(gNorm, TexCoords).rgb;
    vec3 m = texture(gMaterial, TexCoords).rgb;

    material.rough = max(m.r, 0.04);
    material.metal = m.g;
    material.debugging_geometry = m.b;

    vec3 viewFragPos = (u_V * vec4(fragPos, 1.0)).xyz;
    vec3 viewNormal = mat3(u_V) * normalize(normal);

    vec3 color = vec3(0);

    // if (material.alwaysLit == 0.0) {
    // Direct lighting
    vec3 directLighting = vec3(0);
    for (int i = 0; i < int(u_NumLights); i++) {
        vec3 light_contrib = microfacetModel(viewFragPos, viewNormal, u_Lights[i]);

        if (u_Lights[i].shadowMapIdx > -0.5) {
            float shadow_intensity = shadowIntensity(int(u_Lights[i].shadowMapIdx), fragPos, normal, u_Lights[i]);
            float light_intensity = 1.0 - shadow_intensity;
            light_contrib *= light_intensity;
        }

        directLighting += light_contrib;
    }
    
    // Ambient/IBL
    vec3 ambient = vec3(0);
    if (u_UseIbl > 0.5) {
        // IBL
        vec3 N = normalize(normal);

        // if (IBL_FLIP_Y_AXIS) {
        //     N.y = -N.y;
        // }

        int dbg = int(DEBUG_TYPE);

        if (dbg == IBL_DEBUG_TYPE_BASE)
        {
            vec3 irradiance = texture(irradianceMap, N).rgb;
            FragColor = vec4(irradiance, 1.0);
            return;
        }

        vec3 V = normalize(u_CamPos - fragPos);
        vec3 R = reflect(-V, N);

        if (IBL_FLIP_Y_AXIS) {
            R.y = -R.y;
        }


        // DEBUG: Show the prefilter map. should look like perfect mirror
        if (dbg == IBL_DEBUG_TYPE_BASE+1)
        {
            vec3 prefilteredColor = textureLod(prefilterMap, R, 0.0).rgb;  // Mip 0 = sharpest
            FragColor = vec4(prefilteredColor, 1.0);
            return;
        }
        
        vec3 F0 = vec3(0.04);
        F0 = mix(F0, material.color, material.metal);
        
        float NdotV = max(dot(N, V), 0.001);

        // debug brdf lut. should look like gradient red/orange
        if (dbg == IBL_DEBUG_TYPE_BASE+2)
        {
            vec2 brdf = texture(brdfLut, vec2(NdotV, material.rough)).rg;
            FragColor = vec4(brdf.r, brdf.g, 0.0, 1.0);
            return;
        }


        vec3 F = fresnelSchlickRoughness(NdotV, F0, material.rough);
        
        // Diffuse component
        vec3 kD = (1.0 - F) * (1.0 - material.metal);
        vec3 irradiance = texture(irradianceMap, N).rgb;
        // irradiance = irradiance / (irradiance + vec3(1.0));
        vec3 diffuse = kD * irradiance * material.color;

// #define DEBUG_IBL_DIFFUSE
#ifdef DEBUG_IBL_DIFFUSE
        FragColor = vec4(diffuse, 1.0);
        return;
#endif

        // Specular component  
        const float MAX_REFLECTION_LOD = 4.0;

        vec3 prefilteredColor = vec3(0, 0, 0);

#ifdef DEBUG_MIP_INTERPOLATION
        {
            // somehow flooring the mipLevel, no interpolation fixes aura issue?
            // !TODO: jspoh figure out why and a proper fix
            float mipLevel = material.rough * MAX_REFLECTION_LOD;
            mipLevel = floor(mipLevel); // Force discrete mip levels, no interpolation
            prefilteredColor = textureLod(prefilterMap, R, mipLevel).rgb;
        }
#endif

        prefilteredColor = textureLod(prefilterMap, R, material.rough * MAX_REFLECTION_LOD).rgb;

#ifdef DEBUG_MIP
        {
            // debug to see if mip issue
            prefilteredColor = textureLod(prefilterMap, R, 0.0).rgb;  // Force mip 0
        }
#endif

        // prefilteredColor = prefilteredColor / (prefilteredColor + vec3(1.0));  // Reinhard tone mapping

        vec2 envBRDF = texture(brdfLut, vec2(NdotV, material.rough)).rg;
        vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

        ambient = diffuse + specular;
    } else {
        ambient = material.color * u_AmbientLight;
    }
    
    color = directLighting + ambient;

    if (material.debugging_geometry > 0.5) {
        color = material.color;
    }

    vec3 emission = texture(gEmission, TexCoords).rgb;
    color += emission;
    
    // color = color / (color + vec3(1.0)); // Reinhard
    FragColor = vec4(color, 1.0);
}
