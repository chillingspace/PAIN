#version 300 es
precision highp float;
precision highp int;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

layout(location = 3) in ivec4 aBoneIndices;      // the indices of the bones that affect this particular vertex
layout(location = 4) in vec4 aBoneWeights;       // how much effect does this bone have on this vertex.
// in this case, there are a max of 4 bones that can affect a vertex. all 4 bones must add up to 1

// normal map stuff
layout(location = 5) in vec3 aTangent;
layout(location = 6) in vec3 aBitangent;

// instanced model matrix (locations 7-10, divisor=1)
layout(location = 7)  in vec4 aInstanceCol0;
layout(location = 8)  in vec4 aInstanceCol1;
layout(location = 9)  in vec4 aInstanceCol2;
layout(location = 10) in vec4 aInstanceCol3;

out vec3 vNormal;
out vec3 vFragPos;
out vec2 vTexCoords;
out mat3 TBN;

uniform mat4 u_M;
uniform mat4 u_V;
uniform mat4 u_P;

uniform float u_InvertUvY;
uniform float u_Instanced;

// animation
const int MAX_BONES = 100;
uniform mat4 u_BoneMatrices[MAX_BONES];     // xform matrix for each bone
uniform float u_Animated;

void main() {
    mat4 modelMatrix = u_Instanced > 0.5
        ? mat4(aInstanceCol0, aInstanceCol1, aInstanceCol2, aInstanceCol3)
        : u_M;

    vec4 localPos;
    vec3 localNormal;

    if (u_Animated > 0.5) {
        mat4 skin = u_BoneMatrices[aBoneIndices.x] * aBoneWeights.x
                  + u_BoneMatrices[aBoneIndices.y] * aBoneWeights.y
                  + u_BoneMatrices[aBoneIndices.z] * aBoneWeights.z
                  + u_BoneMatrices[aBoneIndices.w] * aBoneWeights.w;
        localPos = skin * vec4(aPos, 1.0);
        localNormal = mat3(skin) * aNormal;
    } else {
        localPos = vec4(aPos, 1.0);
        localNormal = aNormal;
    }

    vNormal = mat3(transpose(inverse(modelMatrix))) * localNormal;
    vFragPos = vec3(modelMatrix * localPos);

    if (u_InvertUvY > 0.5) {
        // invert tex coords vertically for compressed textures(astc)
        vTexCoords = vec2(aTexCoords.x, 1.0 - aTexCoords.y);
    }
    else {
        vTexCoords = vec2(aTexCoords.x, aTexCoords.y);
    }

    mat4 MVP = u_P * u_V * modelMatrix;
    gl_Position = MVP * localPos;

    vec3 T = normalize(vec3(modelMatrix * vec4(aTangent, 0.0)));
    vec3 B = normalize(vec3(modelMatrix * vec4(aBitangent, 0.0)));
    vec3 N = normalize(vec3(modelMatrix * vec4(aNormal, 0.0)));
    TBN = mat3(T, B, N);
}
