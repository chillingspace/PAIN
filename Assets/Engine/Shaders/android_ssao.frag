#version 300 es
precision highp float;
precision highp sampler2D;

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D gPos;
uniform sampler2D gNorm;
uniform sampler2D u_Noise;

#define KERNEL_SIZE 16
uniform vec3 u_Kernel[KERNEL_SIZE];
uniform mat4 u_V;
uniform mat4 u_P;
uniform vec2 u_ScreenSize;
uniform float u_Radius;
uniform float u_Bias;

void main() {
    vec3 worldPos    = texture(gPos,  TexCoords).rgb;
    vec3 worldNormal = texture(gNorm, TexCoords).rgb;

    if (dot(worldNormal, worldNormal) < 0.01) {
        FragColor = vec4(1.0);
        return;
    }

    vec3 fragPos = vec3(u_V * vec4(worldPos, 1.0));
    vec3 normal  = normalize(mat3(u_V) * normalize(worldNormal));

    vec2 noiseUV = TexCoords * u_ScreenSize / 4.0;
    vec3 randomVec = vec3(texture(u_Noise, noiseUV).rg, 0.0);

    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < KERNEL_SIZE; ++i) {
        vec3 samplePos = fragPos + TBN * u_Kernel[i] * u_Radius;

        vec4 offset = u_P * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz  = offset.xyz * 0.5 + 0.5;

        if (offset.x < 0.0 || offset.x > 1.0 ||
            offset.y < 0.0 || offset.y > 1.0)
            continue;

        vec3 sampleWorldPos = texture(gPos, offset.xy).rgb;
        float sampleDepth   = (u_V * vec4(sampleWorldPos, 1.0)).z;

        float rangeCheck = smoothstep(0.0, 1.0, u_Radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + u_Bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - occlusion / float(KERNEL_SIZE);
    FragColor = vec4(vec3(occlusion), 1.0);
}
