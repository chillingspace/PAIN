#version 300 es
precision highp float;
precision highp int;

in vec2 TexCoords;
layout(location = 0) out vec4 FragColor;

uniform sampler2D u_DepthTex;
uniform sampler2D u_HistoryTex;

struct Light {
    vec3 position;
    vec3 L;
    mat4 V;
    mat4 P;
    float shadowMapIdx;
    float type;
    vec3 direction;
    float innerCutoff;
    float outerCutoff;
};

#define MAX_LIGHTS 4
#define MAX_SHADOWMAPPED_LIGHTS 4
#define MAX_VOLUMETRIC_STEPS 64

uniform Light u_Lights[MAX_LIGHTS];
uniform int u_NumLights;
uniform sampler2D u_ShadowMaps[MAX_SHADOWMAPPED_LIGHTS];

uniform vec3 u_CamPos;
uniform mat4 u_InvVP;
uniform mat4 u_PrevVP;

uniform float u_VolumetricIntensity;
uniform int u_VolumetricSteps;
uniform float u_VolumetricMaxDist;
uniform float u_VolumetricScatter;
uniform float u_VolumetricJitterStrength;
uniform float u_HistoryBlend;
uniform float u_HistoryClamp;
uniform int u_HistoryValid;
uniform int u_FrameIndex;

float interleavedGradientNoise(vec2 pixel, float frameIndex) {
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(pixel + frameIndex, magic.xy)));
}

float hgPhase(float cosTheta, float g) {
    float g2 = g * g;
    float denom = pow(max(1.0 + g2 - 2.0 * g * cosTheta, 0.0001), 1.5);
    return (1.0 - g2) / (4.0 * 3.14159265 * denom);
}

float luminance(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

vec3 reconstructWorldPosition(vec2 uv, float depth) {
    vec2 ndcXY = uv * 2.0 - 1.0;
    float ndcZ = depth * 2.0 - 1.0;
    vec4 worldPos = u_InvVP * vec4(ndcXY, ndcZ, 1.0);
    return worldPos.xyz / max(worldPos.w, 0.0001);
}

float sampleShadow(int shadowIdx, vec3 worldPos, Light light) {
    vec4 fragPosLight = light.P * light.V * vec4(worldPos, 1.0);
    vec3 projCoords = fragPosLight.xyz / max(fragPosLight.w, 0.0001);
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float shadowDepth = texture(u_ShadowMaps[shadowIdx], projCoords.xy).r;
    if (shadowDepth >= 0.99) {
        return 0.0;
    }

    return projCoords.z - 0.005 > shadowDepth ? 1.0 : 0.0;
}

void main() {
    float depth = texture(u_DepthTex, TexCoords).r;

    vec3 worldFar = reconstructWorldPosition(TexCoords, 1.0);
    vec3 rayDir = normalize(worldFar - u_CamPos);

    bool hasSurface = depth < 0.99999;
    vec3 surfaceWorldPos = hasSurface ? reconstructWorldPosition(TexCoords, depth) : worldFar;
    float distToSurface = hasSurface ? length(surfaceWorldPos - u_CamPos) : u_VolumetricMaxDist;
    float rayLen = min(distToSurface, u_VolumetricMaxDist);

    int numSteps = clamp(u_VolumetricSteps, 1, MAX_VOLUMETRIC_STEPS);
    float stepSize = rayLen / float(numSteps);
    float jitter = (interleavedGradientNoise(gl_FragCoord.xy, float(u_FrameIndex)) - 0.5) *
                   stepSize * u_VolumetricJitterStrength;

    if (rayLen <= 0.0001 || u_NumLights <= 0) {
        FragColor = vec4(0.0);
        return;
    }

    vec3 accumulated = vec3(0.0);

    for (int s = 0; s < MAX_VOLUMETRIC_STEPS; ++s) {
        if (s >= numSteps) {
            break;
        }

        float t = clamp((float(s) + 0.5) * stepSize + jitter, 0.0, rayLen);
        vec3 samplePos = u_CamPos + rayDir * t;

        for (int i = 0; i < MAX_LIGHTS; ++i) {
            if (i >= u_NumLights) {
                break;
            }

            Light light = u_Lights[i];
            bool hasShadowMap = light.shadowMapIdx > -0.5;

            vec3 lightContrib = vec3(0.0);

            if (int(light.type) == 0) {
                vec3 toLight = light.position - samplePos;
                float dist = length(toLight);
                float attenuation = 100.0 / max(dist * dist, 0.001);
                vec3 toLightNorm = normalize(toLight);
                float inShadow = hasShadowMap ? sampleShadow(int(light.shadowMapIdx), samplePos, light) : 0.0;
                float cosTheta = dot(toLightNorm, -rayDir);
                float phase = hgPhase(cosTheta, u_VolumetricScatter);

                lightContrib = light.L * attenuation * phase * (1.0 - inShadow);
            } else if (int(light.type) == 1) {
                vec3 lightDir = normalize(-light.direction);
                if (!hasShadowMap) {
                    continue;
                }
                float inShadow = sampleShadow(int(light.shadowMapIdx), samplePos, light);
                float cosTheta = dot(lightDir, -rayDir);
                float phase = hgPhase(cosTheta, u_VolumetricScatter);

                lightContrib = light.L * phase * (1.0 - inShadow);
            } else if (int(light.type) == 2) {
                vec3 toLight = light.position - samplePos;
                float dist = length(toLight);
                float attenuation = 100.0 / max(dist * dist, 0.001);
                vec3 toLightNorm = normalize(toLight);
                vec3 spotDir = normalize(light.direction);
                float theta = dot(-toLightNorm, spotDir);
                float spotIntensity = smoothstep(light.outerCutoff, light.innerCutoff, theta);
                float inShadow = hasShadowMap ? sampleShadow(int(light.shadowMapIdx), samplePos, light) : 0.0;
                float cosTheta = dot(toLightNorm, -rayDir);
                float phase = hgPhase(cosTheta, u_VolumetricScatter);

                lightContrib = light.L * attenuation * spotIntensity * phase * (1.0 - inShadow);
            }

            accumulated += lightContrib * stepSize;
        }
    }

    vec3 currentColor = accumulated * u_VolumetricIntensity;
    vec3 finalColor = currentColor;
    if (hasSurface && u_HistoryValid > 0 && u_HistoryBlend > 0.0) {
        vec4 prevClip = u_PrevVP * vec4(surfaceWorldPos, 1.0);
        if (prevClip.w > 0.0001) {
            vec2 prevUV = (prevClip.xy / prevClip.w) * 0.5 + 0.5;
            if (all(greaterThanEqual(prevUV, vec2(0.0))) && all(lessThanEqual(prevUV, vec2(1.0)))) {
                vec3 historyColor = texture(u_HistoryTex, prevUV).rgb;
                vec3 clampExtent = max(abs(currentColor) * u_HistoryClamp, vec3(0.03));
                vec3 clampedHistory = clamp(historyColor, currentColor - clampExtent, currentColor + clampExtent);
                float currLum = luminance(currentColor);
                float histLum = luminance(clampedHistory);
                float lumDelta = abs(histLum - currLum) / max(max(currLum, histLum), 0.05);
                float rejection = 1.0 - smoothstep(0.2, 0.9, lumDelta);
                float historyWeight = u_HistoryBlend * rejection;
                finalColor = mix(currentColor, clampedHistory, historyWeight);
            }
        }
    }

    FragColor = vec4(finalColor, 1.0);
}
