#version 300 es
precision highp float;

out vec4 FragColor;
in vec3 WorldPos;

uniform samplerCube environmentMap;

const float PI = 3.14159265359;
const float EPSILON = 0.0001;
const int PHI_SAMPLES = 64;
const int THETA_SAMPLES = 32;

void main()
{
    vec3 N = normalize(WorldPos);

#ifdef DEBUG
    FragColor = vec4(texture(environmentMap, N).rgb, 1.0);
    return;
#endif

    vec3 irradiance = vec3(0.0);
    // Build a robust tangent basis to avoid NaNs when N aligns with world-up.
    vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(0.0, 0.0, 1.0);
    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    float sampleCount = 0.0;

    // Use fixed integer loop counts for stable behavior across mobile GLSL compilers.
    for (int phiIdx = 0; phiIdx < PHI_SAMPLES; ++phiIdx) {
        float phi = (float(phiIdx) + 0.5) * (2.0 * PI / float(PHI_SAMPLES));
        for (int thetaIdx = 0; thetaIdx < THETA_SAMPLES; ++thetaIdx) {
            float theta = (float(thetaIdx) + 0.5) * (0.5 * PI / float(THETA_SAMPLES));
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec = normalize(
                tangentSample.x * right + tangentSample.y * up + tangentSample.z * N
            );
            float sampleWeight = max(cos(theta) * sin(theta), 0.0);
            irradiance += textureLod(environmentMap, sampleVec, 0.0).rgb * sampleWeight;
            sampleCount += 1.0;
        }
    }

    irradiance = PI * irradiance / max(sampleCount, EPSILON);
    FragColor = vec4(irradiance, 1.0);
}
