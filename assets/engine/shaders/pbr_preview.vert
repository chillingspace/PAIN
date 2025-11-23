#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out mat3 TBN;

uniform mat4 u_M;
uniform mat4 u_V;
uniform mat4 u_P;

void main() {
    FragPos = vec3(u_M * vec4(aPos, 1.0));
    
    //Flip V coordinate here
    TexCoords = vec2(aTexCoords.x, 1.0 - aTexCoords.y);
    
    mat3 normalMatrix = transpose(inverse(mat3(u_M)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    
    TBN = mat3(T, B, N);
    Normal = N;
    
    gl_Position = u_P * u_V * u_M * vec4(aPos, 1.0);
}
