// geometry.vert

#version 330 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_uniform_location : enable

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

// int because opengl has beef with ints istg
layout(location=3) in ivec4 aBoneIndices;       // the indices of the bones that affect this particular vertex
layout(location=4) in vec4 aBoneWeights;        // how much effect does this bone have on this vertex.
// in this case, there are a max of 4 bones that can affect a vertex. all 4 bones must add up to 1

// normal map stuff
layout(location=5) in vec3 aTangent;
layout(location=6) in vec3 aBitangent;

// per-instance model matrix (instanced rendering)
layout(location = 7) in vec4 aInstanceM0;
layout(location = 8) in vec4 aInstanceM1;
layout(location = 9) in vec4 aInstanceM2;
layout(location = 10) in vec4 aInstanceM3;

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec3 vFragPos;
layout(location = 2) out vec2 vTexCoords;
layout(location = 3) out mat3 TBN;

layout(location = 0) uniform mat4 u_M;
layout(location = 1) uniform mat4 u_V;
layout(location = 2) uniform mat4 u_P;

uniform float u_InvertUvY;
uniform float u_UseInstancing;

// animation
const int MAX_BONES = 100;
uniform mat4 u_BoneMatrices[MAX_BONES];     // xform matrix for each bone
uniform float u_Animated;

void main() {
    mat4 effectiveM = u_UseInstancing > 0.5
        ? mat4(aInstanceM0, aInstanceM1, aInstanceM2, aInstanceM3)
        : u_M;

    vec4 localPos;
    vec3 localNormal;

    if (u_Animated > 0.5) {
        mat4 skin = u_BoneMatrices[int(aBoneIndices.x)] * aBoneWeights.x
                  + u_BoneMatrices[int(aBoneIndices.y)] * aBoneWeights.y
                  + u_BoneMatrices[int(aBoneIndices.z)] * aBoneWeights.z
                  + u_BoneMatrices[int(aBoneIndices.w)] * aBoneWeights.w;

        // vec4 hardcodedWeights = vec4(0.25, 0.25, 0.25, 0.25);
        // ivec4 hardcodedIndices = ivec4(0, 0, 0, 0);  // All bone 0
        
        // skin = u_BoneMatrices[hardcodedIndices[0]] * hardcodedWeights[0]
        //           + u_BoneMatrices[hardcodedIndices[1]] * hardcodedWeights[1]
        //           + u_BoneMatrices[hardcodedIndices[2]] * hardcodedWeights[2]
        //           + u_BoneMatrices[hardcodedIndices[3]] * hardcodedWeights[3];
        
        localPos = skin * vec4(aPos, 1.0);
        localNormal = mat3(skin) * aNormal;
    } else {
        localPos = vec4(aPos, 1.0);
        localNormal = aNormal;
    }

    vNormal = mat3(transpose(inverse(effectiveM))) * localNormal;
    vFragPos = vec3(effectiveM * localPos);

    if (u_InvertUvY > 0.5) {
        // invert tex coords vertically for compressed textures(astc)
        vTexCoords = vec2(aTexCoords.x, 1.0 - aTexCoords.y);
    }
    else {
        vTexCoords = vec2(aTexCoords.x, aTexCoords.y);
    }

    mat4 MVP = u_P * u_V * effectiveM;
    gl_Position = MVP * localPos;

    vec3 T = normalize(vec3(effectiveM * vec4(aTangent, 0.0)));
    vec3 B = normalize(vec3(effectiveM * vec4(aBitangent, 0.0)));
    vec3 N = normalize(vec3(effectiveM * vec4(aNormal, 0.0)));
    TBN = mat3(T, B, N);
}
