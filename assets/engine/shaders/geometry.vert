// geometry.vert

#version 330 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_uniform_location : enable

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location=3) in ivec4 aBoneIndices;       // the indices of the bones that affect this particular vertex
layout(location=4) in vec4 aBoneWeights;        // how much effect does this bone have on this vertex.
// in this case, there are a max of 4 bones that can affect a vertex. all 4 bones must add up to 1


layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec3 vFragPos;
layout(location = 2) out vec2 vTexCoords;

layout(location = 0) uniform mat4 u_M;
layout(location = 1) uniform mat4 u_V;
layout(location = 2) uniform mat4 u_P;

uniform float u_InvertUvY;

// animation
const int MAX_BONES = 100;
uniform mat4 u_BoneMatrices[MAX_BONES];     // xform matrix for each bone
uniform float u_Animated;

void main() {
    vec4 localPos;
    vec3 localNormal;
    
    if (u_Animated > 0.5) {
        mat4 skin = u_BoneMatrices[aBoneIndices[0]] * aBoneWeights[0]
                  + u_BoneMatrices[aBoneIndices[1]] * aBoneWeights[1]
                  + u_BoneMatrices[aBoneIndices[2]] * aBoneWeights[2]
                  + u_BoneMatrices[aBoneIndices[3]] * aBoneWeights[3];
        
        localPos = skin * vec4(aPos, 1.0);
        localNormal = mat3(skin) * aNormal;
    } else {
        localPos = vec4(aPos, 1.0);
        localNormal = aNormal;
    }

    vNormal = mat3(transpose(inverse(u_M))) * localNormal;
    vFragPos = vec3(u_M * localPos);

    if (u_InvertUvY > 0.5) {
        // invert tex coords vertically for compressed textures(astc)
        vTexCoords = vec2(aTexCoords.x, 1.0 - aTexCoords.y);
    }
    else {
        vTexCoords = vec2(aTexCoords.x, aTexCoords.y);
    }

    mat4 MVP = u_P * u_V * u_M;
    gl_Position = MVP * localPos;
}
