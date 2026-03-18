#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;
precision highp samplerCube;

#define PI 3.14159265359

in vec2 TexCoords;
layout(location = 0) out vec4 FragColor;

struct Material {
    float rough;
    float metal;
    vec3 color;
    float ao;
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
uniform float u_IblDiffuseStrength;
uniform float u_IblSpecularStrength;
uniform float u_IblMaxReflectionLod;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLut;
uniform vec3 u_CamPos;

#define MAX_SHADOWMAPPED_LIGHTS 4
uniform sampler2D u_ShadowMap0;
uniform sampler2D u_ShadowMap1;
uniform sampler2D u_ShadowMap2;
uniform sampler2D u_ShadowMap3;
uniform float u_NumShadowMaps;

Material material;

// debug
uniform float DEBUG_TYPE;


float ggxDistribution(float nDotH) {
    float alpha = max(material.rough * material.rough, 0.04);
    float alpha2 = alpha * alpha;
    float d = (nDotH * nDotH) * (alpha2 - 1.0) + 1.0;
    return alpha2 / max(PI * d * d, 0.0001);
}

float geometrySchlickGGX(float nDotX) {
    float r = material.rough + 1.0;
    float k = (r * r) / 8.0;
    float denom = nDotX * (1.0 - k) + k;
    return nDotX / max(denom, 0.0001);
}

float geometrySmith(float nDotV, float nDotL) {
    return geometrySchlickGGX(nDotV) * geometrySchlickGGX(nDotL);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// for ibl. schlickFresnel but with roughness
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
} 

float SampleShadowMap(int shadow_map_idx, vec2 uv) {
    if (shadow_map_idx == 0) return texture(u_ShadowMap0, uv).r;
    if (shadow_map_idx == 1) return texture(u_ShadowMap1, uv).r;
    if (shadow_map_idx == 2) return texture(u_ShadowMap2, uv).r;
    return texture(u_ShadowMap3, uv).r;
}

vec2 ShadowTexelSize(int shadow_map_idx) {
    if (shadow_map_idx == 0) return 1.0 / vec2(textureSize(u_ShadowMap0, 0));
    if (shadow_map_idx == 1) return 1.0 / vec2(textureSize(u_ShadowMap1, 0));
    if (shadow_map_idx == 2) return 1.0 / vec2(textureSize(u_ShadowMap2, 0));
    return 1.0 / vec2(textureSize(u_ShadowMap3, 0));
}

vec3 microfacetModel(vec3 position, vec3 n, Light light) {
    vec3 l;
    vec3 radiance = light.L;
    
    if (int(light.type) == 0) { 
        vec3 lightPositionInView = (u_V * vec4(light.position, 1.0)).xyz;
        l = lightPositionInView - position;
        float dist = length(l);
        l = normalize(l);
        radiance *= 100.0 / max(dist * dist, 0.001);
    }
    else if (int(light.type) == 1) {
        vec3 lightDirView = mat3(u_V) * normalize(-light.direction);
        l = normalize(lightDirView);
    }
    else if (int(light.type) == 2) {
        vec3 lightPositionInView = (u_V * vec4(light.position, 1.0)).xyz;
        l = lightPositionInView - position;
        float dist = length(l);
        l = normalize(l);
        radiance *= 100.0 / max(dist * dist, 0.001);

        vec3 spotDirView = normalize(mat3(u_V) * light.direction); 
        float theta = dot(-l, spotDirView); 
        float epsilon = max(light.innerCutoff - light.outerCutoff, 0.001);
        float spotIntensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
        radiance *= spotIntensity;
    }

    vec3 v = normalize(-position);
    vec3 h = normalize(v + l);
    float nDotH = max(dot(n, h), 0.0);
    float hDotV = max(dot(h, v), 0.0);
    float nDotL = max(dot(n, l), 0.0);
    float nDotV = max(dot(n, v), 0.0);

    if (nDotL <= 0.0 || nDotV <= 0.0) {
        return vec3(0.0);
    }

    vec3 F0 = mix(vec3(0.04), material.color, material.metal);
    vec3 F = fresnelSchlick(hDotV, F0);
    float D = ggxDistribution(nDotH);
    float G = geometrySmith(nDotV, nDotL);
    vec3 specular = (D * G * F) / max(4.0 * nDotV * nDotL, 0.0001);

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - material.metal);
    vec3 diffuse = kD * material.color / PI;

    return (diffuse + specular) * radiance * nDotL;
}

float shadowIntensity(int shadow_map_idx, vec3 fragPos, vec3 normal, Light light) {
    vec4 fragPosLight = light.P * light.V * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLight.xyz / fragPosLight.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float shadow_map_depth = SampleShadowMap(shadow_map_idx, projCoords.xy);

    
    float frag_depth = projCoords.z;
    
    // If shadow map is empty (cleared to 1.0), no shadows
    if (shadow_map_depth >= 0.99) {
        return 0.0; // No shadow
    }
    // Bias should follow actual light direction. Using the camera-follow shadow source
    // position for directional lights makes the bias unstable and causes shimmering.
    vec3 light_dir = int(light.type) == 1
        ? normalize(-light.direction)
        : normalize(light.position - fragPos);
    float bias = max(0.02 * (1.0 - dot(normal, light_dir)), 0.002);

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
const int IBL_DEBUG_IRRADIANCE = IBL_DEBUG_TYPE_BASE;
const int IBL_DEBUG_PREFILTER = IBL_DEBUG_TYPE_BASE + 1;
const int IBL_DEBUG_BRDFLUT = IBL_DEBUG_TYPE_BASE + 2;
const int IBL_DIFFUSE = IBL_DEBUG_TYPE_BASE + 3;
const int IBL_SPECULAR = IBL_DEBUG_TYPE_BASE + 4;
const int DIRECT_LIGHTING = IBL_DEBUG_TYPE_BASE + 5;
const bool IBL_FLIP_Y_AXIS = true;
const float IBL_SPECULAR_FIRELFY_CLAMP = 32.0;

bool IsFiniteVec3(vec3 v) {
    return !(any(isnan(v)) || any(isinf(v)));
}

vec3 SanitizeIblSample(vec3 value) {
    if (!IsFiniteVec3(value)) {
        return vec3(0.0);
    }
    return max(value, vec3(0.0));
}

void main() {
    if (int(u_NumShadowMaps) > MAX_SHADOWMAPPED_LIGHTS) {
        FragColor = vec4(1.0, 0.0, 1.0, 1.0);
        return;
    }


    vec3 fragPos = texture(gPos, TexCoords).rgb;
    material.color = texture(gCol, TexCoords).rgb;
    vec3 normal = texture(gNorm, TexCoords).rgb;
    vec3 m = texture(gMaterial, TexCoords).rgb;

    material.rough = clamp(m.r, 0.04, 1.0);
    material.metal = clamp(m.g, 0.0, 1.0);
    material.ao = clamp(m.b, 0.0, 1.0);

    vec3 viewFragPos = (u_V * vec4(fragPos, 1.0)).xyz;
    vec3 viewNormal = mat3(u_V) * normalize(normal);

    vec3 color = vec3(0.0);
    int dbg = int(DEBUG_TYPE);

    // if (material.alwaysLit == 0.0) {
    // Direct lighting
    vec3 directLighting = vec3(0.0);
    for (int i = 0; i < int(u_NumLights); i++) {
        vec3 light_contrib = microfacetModel(viewFragPos, viewNormal, u_Lights[i]);

        if (u_Lights[i].shadowMapIdx > -0.5) {
            float shadow_intensity = shadowIntensity(int(u_Lights[i].shadowMapIdx), fragPos, normal, u_Lights[i]);
            float light_intensity = 1.0 - shadow_intensity;
            light_contrib *= light_intensity;
        }

        directLighting += light_contrib;
    }
    
    if (dbg == DIRECT_LIGHTING) {
        FragColor = vec4(directLighting, 1.0);
        return;
    }

    // Ambient/IBL
    vec3 ambient = vec3(0.0);
    bool wantsIblDebug = dbg >= IBL_DEBUG_IRRADIANCE && dbg <= IBL_SPECULAR;
    if (wantsIblDebug && !(u_UseIbl > 0.5)) {
        // Explicit missing-IBL marker in debug modes instead of silently falling back.
        FragColor = vec4(1.0, 0.0, 1.0, 1.0);
        return;
    }
    if (u_UseIbl > 0.5) {
        // IBL
        vec3 N = normalize(normal);

        // if (IBL_FLIP_Y_AXIS) {
        //     N.y = -N.y;
        // }

        if (dbg == IBL_DEBUG_IRRADIANCE)
        {
            vec3 irradiance = SanitizeIblSample(texture(irradianceMap, N).rgb);
            FragColor = vec4(irradiance, 1.0);
            return;
        }

        vec3 V = normalize(u_CamPos - fragPos);
        vec3 R = reflect(-V, N);

        if (IBL_FLIP_Y_AXIS) {
            R.y = -R.y;
        }


        // DEBUG: Show the prefilter map. should look like perfect mirror
        if (dbg == IBL_DEBUG_PREFILTER)
        {
            vec3 prefilteredColor = SanitizeIblSample(textureLod(prefilterMap, R, 0.0).rgb);  // Mip 0 = sharpest
            FragColor = vec4(prefilteredColor, 1.0);
            return;
        }
        
        vec3 F0 = vec3(0.04);
        F0 = mix(F0, material.color, material.metal);
        
        float NdotV = clamp(dot(N, V), 0.001, 1.0);

        // debug brdf lut. should look like gradient red/orange
        if (dbg == IBL_DEBUG_BRDFLUT)
        {
            vec2 brdf = texture(brdfLut, vec2(NdotV, material.rough)).rg;
            brdf = clamp(brdf, vec2(0.0), vec2(1.0));
            FragColor = vec4(brdf.r, brdf.g, 0.0, 1.0);
            return;
        }


        vec3 F = fresnelSchlickRoughness(NdotV, F0, material.rough);
        
        // Diffuse component
        vec3 kD = clamp((1.0 - F) * (1.0 - material.metal), vec3(0.0), vec3(1.0));
        vec3 irradiance = SanitizeIblSample(texture(irradianceMap, N).rgb);
        // irradiance = irradiance / (irradiance + vec3(1.0));
        vec3 diffuse = kD * irradiance * material.color * material.ao * u_IblDiffuseStrength;
        diffuse = SanitizeIblSample(diffuse);

// #define DEBUG_IBL_DIFFUSE
        if (dbg == IBL_DIFFUSE) {
            FragColor = vec4(diffuse, 1.0);
            return;
        }

        // Specular component  

        vec3 prefilteredColor = vec3(0.0);

#ifdef DEBUG_MIP_INTERPOLATION
        {
            // somehow flooring the mipLevel, no interpolation fixes aura issue?
            // !TODO: jspoh figure out why and a proper fix
            float mipLevel = material.rough * u_IblMaxReflectionLod;
            mipLevel = floor(mipLevel); // Force discrete mip levels, no interpolation
            prefilteredColor = SanitizeIblSample(textureLod(prefilterMap, R, mipLevel).rgb);
        }
#endif

        prefilteredColor = SanitizeIblSample(textureLod(prefilterMap, R, material.rough * u_IblMaxReflectionLod).rgb);

#ifdef DEBUG_MIP
        {
            // debug to see if mip issue
            prefilteredColor = SanitizeIblSample(textureLod(prefilterMap, R, 0.0).rgb);  // Force mip 0
        }
#endif

        // prefilteredColor = prefilteredColor / (prefilteredColor + vec3(1.0));  // Reinhard tone mapping

        vec2 envBRDF = texture(brdfLut, vec2(NdotV, material.rough)).rg;
        envBRDF = clamp(envBRDF, vec2(0.0), vec2(1.0));
        vec3 specularTerm = max(F * envBRDF.x + envBRDF.y, vec3(0.0));
        vec3 specular = prefilteredColor * specularTerm * u_IblSpecularStrength;
        specular = SanitizeIblSample(specular);
        float peak = max(specular.r, max(specular.g, specular.b));
        if (peak > IBL_SPECULAR_FIRELFY_CLAMP) {
            specular *= IBL_SPECULAR_FIRELFY_CLAMP / peak;
        }

        if (dbg == IBL_SPECULAR) {
            FragColor = vec4(specular, 1.0);
            return;
        }

        ambient = diffuse + specular;
    } else {
        ambient = material.color * material.ao * u_AmbientLight;
    }
    
    color = directLighting + ambient;

    if (dbg == 1) {
        color = material.color;
    }

    vec3 emission = texture(gEmission, TexCoords).rgb;
    color += emission;
    
    // color = color / (color + vec3(1.0)); // Reinhard
    FragColor = vec4(color, 1.0);
}
