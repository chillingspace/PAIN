#version 330 core
out vec4 FragColor;
in vec3 WorldPos;

uniform samplerCube environmentMap;
uniform float roughness;

const float PI = 3.14159265359;
const float EPSILON = 0.0001;
const float MAX_REFLECTION_MIP = 9.0;
const float MAX_HDR_RADIANCE = 60000.0;
const float PREFILTER_SAMPLE_LUMA_CLAMP = 128.0;

bool IsFiniteVec3(vec3 v) {
    return !(any(isnan(v)) || any(isinf(v)));
}

vec3 SanitizeHdrSample(vec3 sampleValue, out bool valid) {
    valid = IsFiniteVec3(sampleValue);
    if (!valid) {
        return vec3(0.0);
    }
    return clamp(sampleValue, vec3(0.0), vec3(MAX_HDR_RADIANCE));
}

vec3 ClampLuminance(vec3 value, float maxLuma) {
    const vec3 lumaWeights = vec3(0.2126, 0.7152, 0.0722);
    float luma = dot(value, lumaWeights);
    if (luma > maxLuma && luma > EPSILON) {
        value *= (maxLuma / luma);
    }
    return value;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = max(roughness * roughness, EPSILON);
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = max(PI * denom * denom, EPSILON);

    return nom / denom;
}

// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// Efficient VanDerCorput calculation
float RadicalInverse_VdC(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness * roughness;
    
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    
    // From spherical coordinates to cartesian coordinates - halfway vector
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    
    // From tangent-space H vector to world-space sample vector
    vec3 up          = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

void main()
{		
    vec3 N = normalize(WorldPos);
    
    // Make the simplifying assumption that V equals R equals the normal 
    vec3 R = N;
    vec3 V = R;
    float safeRoughness = max(roughness, EPSILON);

    // Avoid singular GGX/PDF behavior at roughness==0 on some mobile GPUs.
    if (roughness <= EPSILON)
    {
        bool baseSampleValid;
        vec3 baseSample = SanitizeHdrSample(textureLod(environmentMap, R, 0.0).rgb, baseSampleValid);
        FragColor = vec4(baseSample, 1.0);
        return;
    }

    const uint SAMPLE_COUNT = 64u;
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;
    
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        // Generates a sample vector that's biased towards the preferred alignment direction (importance sampling)
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, safeRoughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
            // Sample from the environment's mip level based on roughness/pdf
            float D   = DistributionGGX(N, H, safeRoughness);
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), EPSILON);
            float pdf = D * NdotH / max(4.0 * HdotV, EPSILON);
            if (!(pdf > EPSILON)) {
                continue;
            }

            float resolution = 512.0; // resolution of source cubemap (per face)
            float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + EPSILON);
            float mipLevel = 0.5 * log2(max(saSample / saTexel, EPSILON));
            if (!(mipLevel >= 0.0)) {
                mipLevel = 0.0;
            }
            mipLevel = clamp(mipLevel, 0.0, MAX_REFLECTION_MIP);

            bool envSampleValid;
            vec3 envSample = SanitizeHdrSample(textureLod(environmentMap, L, mipLevel).rgb, envSampleValid);
            if (!envSampleValid) {
                continue;
            }
            envSample = ClampLuminance(envSample, PREFILTER_SAMPLE_LUMA_CLAMP);
            prefilteredColor += envSample * NdotL;
            totalWeight      += NdotL;
        }
    }

    prefilteredColor = prefilteredColor / max(totalWeight, EPSILON);

    FragColor = vec4(prefilteredColor, 1.0);
}
