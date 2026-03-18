// geometry.frag

#version 330 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_uniform_location : enable

#define PI 3.14159265359

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec3 vFragPos;
layout(location = 2) in vec2 vTexCoords;
layout(location = 3) in mat3 TBN;

layout(location = 0) out vec3 gPos;
layout(location = 1) out vec3 gCol;
layout(location = 2) out vec3 gNorm;
layout(location = 3) out vec3 gMaterial;
layout(location = 4) out vec3 gEmission;

struct Material {
    float rough;
    float metal;
    float useTex;
    sampler2D tex;
    float use_ao;
    sampler2D ao_map;
    float use_normal;
    sampler2D normal_map;
    float use_emission;
    sampler2D emission_map;
    float use_roughness;
    sampler2D roughness_map;
    float use_metallic;
    sampler2D metallic_map;
    vec3 color;
};

uniform Material material;
uniform vec3 u_EmissionOverride;
uniform float u_UseEmissionOverride;
uniform float DEBUG_TYPE;

const int IBL_DEBUG_TYPE = 8;

bool IsFiniteFloat(float value) {
    return !(isnan(value) || isinf(value));
}

bool IsFiniteVec3(vec3 value) {
    return !(any(isnan(value)) || any(isinf(value)));
}

vec3 SafeNormalize(vec3 value, vec3 fallback) {
    float len2 = dot(value, value);
    if (!IsFiniteFloat(len2) || len2 <= 1e-8) {
        return fallback;
    }
    return value * inversesqrt(len2);
}

vec3 DecodeTangentNormal(vec3 sampleRgb) {
    vec3 raw = clamp(sampleRgb, vec3(0.0), vec3(1.0));
    vec3 decoded = raw * 2.0 - 1.0;
    decoded.xy = clamp(decoded.xy, vec2(-0.999), vec2(0.999));
    float xy2 = dot(decoded.xy, decoded.xy);
    if (xy2 > 1.0) {
        decoded.xy *= inversesqrt(xy2);
        xy2 = dot(decoded.xy, decoded.xy);
    }
    decoded.z = sqrt(max(1.0 - xy2, 0.0));
    return decoded;
}

mat3 BuildStableTBN(vec3 geometricNormal) {
    vec3 N = SafeNormalize(geometricNormal, vec3(0.0, 0.0, 1.0));
    vec3 T = SafeNormalize(TBN[0], vec3(1.0, 0.0, 0.0));
    T = SafeNormalize(T - N * dot(T, N), vec3(1.0, 0.0, 0.0));

    vec3 BRef = SafeNormalize(TBN[1], cross(N, T));
    vec3 B = SafeNormalize(cross(N, T), BRef);
    if (dot(B, BRef) < 0.0) {
        B = -B;
    }

    return mat3(T, B, N);
}

vec3 ResolveGeometryNormal() {
    vec3 geometricNormal = SafeNormalize(vNormal, vec3(0.0, 0.0, 1.0));
    if (material.use_normal > 0.5) {
        mat3 stableTBN = BuildStableTBN(geometricNormal);
        vec3 sampledNormal = texture(material.normal_map, vTexCoords).rgb;

        // Candidate A: texture values are already linear (expected path).
        vec3 tangentLinear = DecodeTangentNormal(sampledNormal);
        vec3 worldLinear = SafeNormalize(stableTBN * tangentLinear, geometricNormal);

        // Candidate B: texture was unintentionally sampled through sRGB decode.
        vec3 tangentFromSrgb = DecodeTangentNormal(pow(clamp(sampledNormal, vec3(0.0), vec3(1.0)), vec3(1.0 / 2.2)));
        vec3 worldFromSrgb = SafeNormalize(stableTBN * tangentFromSrgb, geometricNormal);

        vec3 worldNormal = dot(worldLinear, geometricNormal) >= dot(worldFromSrgb, geometricNormal)
            ? worldLinear
            : worldFromSrgb;

        // Never allow inward/invalid normals: they produce black irradiance patches.
        if (!IsFiniteVec3(worldNormal) || dot(worldNormal, geometricNormal) <= 0.0) {
            return geometricNormal;
        }
        return worldNormal;
    }
    return geometricNormal;
}

vec3 ResolveGeometryMaterial(int dbg) {
    float roughness = material.rough;
    if (material.use_roughness > 0.5) {
        roughness = texture(material.roughness_map, vTexCoords).r;
    }
    roughness = clamp(roughness, 0.04, 1.0);

    float metallic = material.metal;
    if (material.use_metallic > 0.5) {
        metallic = texture(material.metallic_map, vTexCoords).r;
    }
    metallic = clamp(metallic, 0.0, 1.0);

    float ao = material.use_ao > 0.5 ? texture(material.ao_map, vTexCoords).r : 1.0;
    ao = clamp(ao, 0.0, 1.0);

    return vec3(roughness, metallic, ao);
}

vec3 ResolveGeometryEmission() {
    if (u_UseEmissionOverride > 0.5) {
        return u_EmissionOverride;
    }
    if (material.use_emission > 0.5) {
        return texture(material.emission_map, vTexCoords).rgb;
    }
    return vec3(0.0);
}

vec3 ResolveGeometryColor() {
    if (material.useTex == 0.0) {
        return material.color;
    }

    return pow(texture(material.tex, vTexCoords).rgb, vec3(2.2));
}

void main() {
    int dbg = int(DEBUG_TYPE);
    gPos = vFragPos;
    gNorm = ResolveGeometryNormal();
    gMaterial = ResolveGeometryMaterial(dbg);
    gEmission = ResolveGeometryEmission();
    gCol = ResolveGeometryColor();

    if (dbg > 0 && dbg < IBL_DEBUG_TYPE) {
        if (dbg == 1 || dbg == 2) gCol = ResolveGeometryColor();
        else if (dbg == 3) gCol = vec3(gMaterial.b);
        else if (dbg == 4) gCol = material.use_normal > 0.5 ? texture(material.normal_map, vTexCoords).rgb : normalize(vNormal) * 0.5 + 0.5;
        else if (dbg == 5) gCol = vec3(gMaterial.r);
        else if (dbg == 6) gCol = vec3(gMaterial.g);
        else if (dbg == 7) gCol = gEmission;
        return;
    }
}
