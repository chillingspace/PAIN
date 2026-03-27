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
struct GPULight {
    vec4 position_type;      // xyz + type
    vec4 intensity_shadow;   // rgb + shadow map index
    vec4 direction_inner;    // xyz + inner cutoff
    vec4 outer_padding;      // x = outer cutoff
    mat4 V;
    mat4 P;
};
layout(std140) uniform PbrLightBlock {
    GPULight uboLights[MAX_LIGHTS];
};
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
uniform float u_IblRoughnessBias;
uniform float u_IblSpecularMipBias;
uniform float u_IblSpecularStrengthScale;
uniform float u_IblSpecularPrefilterLumaClamp;
uniform float u_IblSpecularFireflyClamp;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLut;
uniform vec3 u_CamPos;

#define MAX_SHADOWMAPPED_LIGHTS 4
uniform sampler2D u_ShadowMaps[MAX_SHADOWMAPPED_LIGHTS];
uniform float u_NumShadowMaps;

Material material = Material(0.5, 0.0, vec3(1.0), 1.0);

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
    if (shadow_map_idx == 0) return texture(u_ShadowMaps[0], uv).r;
    if (shadow_map_idx == 1) return texture(u_ShadowMaps[1], uv).r;
    if (shadow_map_idx == 2) return texture(u_ShadowMaps[2], uv).r;
    if (shadow_map_idx == 3) return texture(u_ShadowMaps[3], uv).r;
    return 1.0;
}

vec2 ShadowTexelSize(int shadow_map_idx) {
    if (shadow_map_idx == 0) return 1.0 / vec2(textureSize(u_ShadowMaps[0], 0));
    if (shadow_map_idx == 1) return 1.0 / vec2(textureSize(u_ShadowMaps[1], 0));
    if (shadow_map_idx == 2) return 1.0 / vec2(textureSize(u_ShadowMaps[2], 0));
    if (shadow_map_idx == 3) return 1.0 / vec2(textureSize(u_ShadowMaps[3], 0));
    return vec2(1.0);
}

Light FetchLight(int index) {
    GPULight packedLight = uboLights[index];
    Light light;
    light.position = packedLight.position_type.xyz;
    light.type = packedLight.position_type.w;
    light.L = packedLight.intensity_shadow.xyz;
    light.shadowMapIdx = packedLight.intensity_shadow.w;
    light.direction = packedLight.direction_inner.xyz;
    light.innerCutoff = packedLight.direction_inner.w;
    light.outerCutoff = packedLight.outer_padding.x;
    light.V = packedLight.V;
    light.P = packedLight.P;
    return light;
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
    // frag pos in light space
    vec4 fragPosLight = light.P * light.V * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLight.xyz / fragPosLight.w;    // perspective divide
    projCoords = projCoords * 0.5 + 0.5;        // convert NDC[-1, 1] to UV[0, 1]

    // check if proj coords within shadow map
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
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
    float ndotl = clamp(dot(normal, light_dir), 0.0, 1.0);
    float slope = 1.0 - ndotl;
    // Use slightly higher bias for directional lights to reduce acne from large frustum
    float slopeScale = int(light.type) == 1 ? 0.015 : (int(light.type) == 2 ? 0.008 : 0.012);
    float minBias = int(light.type) == 1 ? 0.002 : (int(light.type) == 2 ? 0.0008 : 0.0012);
    float bias = max(slopeScale * slope, minBias);

    // PCF with Poisson disk sampling for smooth shadow edges
    // 16 samples arranged in a Poisson disk pattern for better distribution
    // Balanced for Windows desktop - good quality with acceptable performance
    vec2 poissonDisk[16];
    poissonDisk[0] = vec2(-0.94201624, -0.39906216);
    poissonDisk[1] = vec2(0.94558609, -0.76890725);
    poissonDisk[2] = vec2(-0.094184101, -0.92938870);
    poissonDisk[3] = vec2(0.34495938, 0.29387760);
    poissonDisk[4] = vec2(-0.91588581, 0.45771432);
    poissonDisk[5] = vec2(-0.81544232, -0.87912464);
    poissonDisk[6] = vec2(-0.38277543, 0.27676845);
    poissonDisk[7] = vec2(0.97484398, 0.75648379);
    poissonDisk[8] = vec2(0.44323325, -0.97511554);
    poissonDisk[9] = vec2(0.53742981, -0.47373420);
    poissonDisk[10] = vec2(-0.26496911, -0.41893023);
    poissonDisk[11] = vec2(0.79197514, 0.19090188);
    poissonDisk[12] = vec2(-0.24188840, 0.99706507);
    poissonDisk[13] = vec2(-0.81409955, 0.91437590);
    poissonDisk[14] = vec2(0.19984126, 0.78641367);
    poissonDisk[15] = vec2(0.14383161, -0.14100790);
    
    float shadow = 0.0;
    vec2 texelSize = ShadowTexelSize(shadow_map_idx);
    
    // Use larger spread for directional lights (they cover more area)
    float spread = int(light.type) == 1 ? 2.0 : 1.5;
    
    for(int i = 0; i < 16; ++i) {
        vec2 sampleOffset = poissonDisk[i] * texelSize * spread;
        float pcfDepth = SampleShadowMap(shadow_map_idx, projCoords.xy + sampleOffset);
        
        if (pcfDepth < 0.999) {
            shadow += frag_depth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 16.0;
    
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
const float SPECULAR_AA_VARIANCE_SCALE = 0.15;
const float SPECULAR_AA_MAX_ADDITION = 0.35;

bool IsFiniteVec3(vec3 v) {
    return !(any(isnan(v)) || any(isinf(v)));
}

vec3 SanitizeIblSample(vec3 value) {
    if (!IsFiniteVec3(value)) {
        return vec3(0.0);
    }
    return max(value, vec3(0.0));
}

vec3 ClampLuminance(vec3 value, float maxLuma) {
    if (maxLuma <= 0.0) {
        return value;
    }
    const vec3 lumaWeights = vec3(0.2126, 0.7152, 0.0722);
    float luma = dot(value, lumaWeights);
    if (luma > maxLuma && luma > 0.0001) {
        value *= (maxLuma / luma);
    }
    return value;
}

float ApplySpecularAA(float roughness, vec3 N) {
    vec3 dndx = dFdx(N);
    vec3 dndy = dFdy(N);
    float variance = 0.5 * (dot(dndx, dndx) + dot(dndy, dndy));
    float aaTerm = clamp(variance * SPECULAR_AA_VARIANCE_SCALE, 0.0, SPECULAR_AA_MAX_ADDITION);
    float filtered = sqrt(clamp(roughness * roughness + aaTerm, 0.0, 1.0));
    return clamp(filtered, 0.04, 1.0);
}

void main() {
    if (u_NumShadowMaps > MAX_SHADOWMAPPED_LIGHTS) {
        FragColor = vec4(1, 0, 1, 1);
        return;
    }


    vec3 fragPos = texture(gPos, TexCoords).rgb;
    material.color = texture(gCol, TexCoords).rgb;
    vec3 normal = texture(gNorm, TexCoords).rgb;
    vec3 m = texture(gMaterial, TexCoords).rgb;

    material.rough = clamp(m.r, 0.04, 1.0);
    material.metal = clamp(m.g, 0.0, 1.0);
    material.ao = clamp(m.b, 0.0, 1.0);

    vec3 shadedNormal = normalize(normal);
    material.rough = ApplySpecularAA(material.rough, shadedNormal);

    vec3 viewFragPos = (u_V * vec4(fragPos, 1.0)).xyz;
    vec3 viewNormal = mat3(u_V) * shadedNormal;

    vec3 color = vec3(0.0);
    int dbg = int(DEBUG_TYPE);

    // if (material.alwaysLit == 0.0) {
    // Direct lighting
    vec3 directLighting = vec3(0.0);
    for (int i = 0; i < int(u_NumLights); i++) {
        Light light = FetchLight(i);
        vec3 light_contrib = microfacetModel(viewFragPos, viewNormal, light);

        if (light.shadowMapIdx > -0.5) {
            float shadow_intensity = shadowIntensity(int(light.shadowMapIdx), fragPos, normal, light);
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
        vec3 N = shadedNormal;

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
        float iblRoughness = clamp(material.rough + u_IblRoughnessBias, 0.04, 1.0);
        
        float NdotV = clamp(dot(N, V), 0.001, 1.0);

        // debug brdf lut. should look like gradient red/orange
        if (dbg == IBL_DEBUG_BRDFLUT)
        {
            vec2 brdf = texture(brdfLut, vec2(NdotV, iblRoughness)).rg;
            brdf = clamp(brdf, vec2(0.0), vec2(1.0));
            FragColor = vec4(brdf.r, brdf.g, 0.0, 1.0);
            return;
        }


        vec3 F = fresnelSchlickRoughness(NdotV, F0, iblRoughness);
        
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
            float mipLevel = iblRoughness * u_IblMaxReflectionLod;
            mipLevel = floor(mipLevel); // Force discrete mip levels, no interpolation
            prefilteredColor = SanitizeIblSample(textureLod(prefilterMap, R, mipLevel).rgb);
        }
#endif

        float reflectionLod = clamp(
            iblRoughness * u_IblMaxReflectionLod + u_IblSpecularMipBias,
            0.0,
            u_IblMaxReflectionLod
        );
        prefilteredColor = SanitizeIblSample(textureLod(prefilterMap, R, reflectionLod).rgb);
        prefilteredColor = ClampLuminance(prefilteredColor, u_IblSpecularPrefilterLumaClamp);

#ifdef DEBUG_MIP
        {
            // debug to see if mip issue
            prefilteredColor = SanitizeIblSample(textureLod(prefilterMap, R, 0.0).rgb);  // Force mip 0
        }
#endif

        // prefilteredColor = prefilteredColor / (prefilteredColor + vec3(1.0));  // Reinhard tone mapping

        vec2 envBRDF = texture(brdfLut, vec2(NdotV, iblRoughness)).rg;
        envBRDF = clamp(envBRDF, vec2(0.0), vec2(1.0));
        vec3 specularTerm = max(F * envBRDF.x + envBRDF.y, vec3(0.0));
        vec3 specular = prefilteredColor * specularTerm * (u_IblSpecularStrength * u_IblSpecularStrengthScale);
        specular = SanitizeIblSample(specular);
        float peak = max(specular.r, max(specular.g, specular.b));
        if (u_IblSpecularFireflyClamp > 0.0 && peak > u_IblSpecularFireflyClamp) {
            specular *= u_IblSpecularFireflyClamp / peak;
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
