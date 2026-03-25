// geometry.vert

#version 330 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_uniform_location : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

layout(location=3) in ivec4 aBoneIndices;
layout(location=4) in vec4 aBoneWeights;
layout(location=5) in vec3 aTangent;
layout(location=6) in vec3 aBitangent;

layout(location=7)  in vec4 aInstanceCol0;
layout(location=8)  in vec4 aInstanceCol1;
layout(location=9)  in vec4 aInstanceCol2;
layout(location=10) in vec4 aInstanceCol3;

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec3 vFragPos;
layout(location = 2) out vec2 vTexCoords;
layout(location = 3) out mat3 TBN;

layout(location = 0) uniform mat4 u_M;
layout(location = 1) uniform mat4 u_V;
layout(location = 2) uniform mat4 u_P;

uniform float u_InvertUvY;
uniform float u_Instanced;

// OPTIMIZATION: Use UBO for bone matrices instead of individual uniforms
// This reduces CPU overhead from 100+ SetUniform calls to a single buffer update
const int MAX_BONES = 100;
layout(std140, binding = 1) uniform BoneBlock {
    mat4 u_BoneMatrices[MAX_BONES];
};

uniform float u_Animated;

mat4 ResolveModelMatrix() {
    return u_Instanced > 0.5
        ? mat4(aInstanceCol0, aInstanceCol1, aInstanceCol2, aInstanceCol3)
        : u_M;
}

mat4 ResolveSkinMatrix() {
    return u_BoneMatrices[int(aBoneIndices.x)] * aBoneWeights.x
         + u_BoneMatrices[int(aBoneIndices.y)] * aBoneWeights.y
         + u_BoneMatrices[int(aBoneIndices.z)] * aBoneWeights.z
         + u_BoneMatrices[int(aBoneIndices.w)] * aBoneWeights.w;
}

vec2 ResolveTexCoords() {
    return u_InvertUvY > 0.5
        ? vec2(aTexCoords.x, 1.0 - aTexCoords.y)
        : aTexCoords;
}

mat3 ResolveTBN(mat4 modelMatrix) {
    vec3 T = normalize(vec3(modelMatrix * vec4(aTangent, 0.0)));
    vec3 B = normalize(vec3(modelMatrix * vec4(aBitangent, 0.0)));
    vec3 N = normalize(vec3(modelMatrix * vec4(aNormal, 0.0)));
    return mat3(T, B, N);
}

void main() {
    mat4 modelMatrix = ResolveModelMatrix();

    vec4 localPos = vec4(aPos, 1.0);
    vec3 localNormal = aNormal;
    if (u_Animated > 0.5) {
        mat4 skin = ResolveSkinMatrix();
        localPos = skin * localPos;
        localNormal = mat3(skin) * localNormal;
    }

    vNormal = mat3(transpose(inverse(modelMatrix))) * localNormal;
    vFragPos = vec3(modelMatrix * localPos);
    vTexCoords = ResolveTexCoords();
    TBN = ResolveTBN(modelMatrix);
    gl_Position = u_P * u_V * modelMatrix * localPos;
}
