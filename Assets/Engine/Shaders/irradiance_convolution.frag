#version 330 core
out vec4 FragColor;
in vec3 WorldPos;

uniform samplerCube environmentMap;

const float PI = 3.14159265359;
const float EPSILON = 0.0001;
const float MAX_HDR_RADIANCE = 60000.0;
const float IRRADIANCE_SAMPLE_LUMA_CLAMP = 64.0;
const int PHI_SAMPLES = 64;
const int THETA_SAMPLES = 32;

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

void main()
{
    vec3 N = normalize(WorldPos);

#ifdef DEBUG
    bool debugValid;
    vec3 debugSample = SanitizeHdrSample(texture(environmentMap, N).rgb, debugValid);
    FragColor = vec4(debugSample, 1.0);
    return;
#endif

    vec3 irradiance = vec3(0.0);
    // Build a robust tangent basis to avoid NaNs when N aligns with world-up.
    vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(0.0, 0.0, 1.0);
    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    float sampleCount = 0.0;

    // Use fixed integer loop counts for stable behavior across desktop/mobile compilers.
    for (int phiIdx = 0; phiIdx < PHI_SAMPLES; ++phiIdx) {
        float phi = (float(phiIdx) + 0.5) * (2.0 * PI / float(PHI_SAMPLES));
        for (int thetaIdx = 0; thetaIdx < THETA_SAMPLES; ++thetaIdx) {
            float theta = (float(thetaIdx) + 0.5) * (0.5 * PI / float(THETA_SAMPLES));
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec = normalize(
                tangentSample.x * right + tangentSample.y * up + tangentSample.z * N
            );
            float sampleWeight = max(cos(theta) * sin(theta), 0.0);
            bool sampleValid;
            // Use implicit LOD here to reduce aliasing from tiny ultra-bright texels.
            vec3 envSample = SanitizeHdrSample(texture(environmentMap, sampleVec).rgb, sampleValid);
            if (!sampleValid) {
                continue;
            }
            envSample = ClampLuminance(envSample, IRRADIANCE_SAMPLE_LUMA_CLAMP);
            irradiance += envSample * sampleWeight;
            sampleCount += 1.0;
        }
    }

    irradiance = PI * irradiance / max(sampleCount, EPSILON);
    FragColor = vec4(irradiance, 1.0);
}
