// particle.vert

#version 330 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_uniform_location : enable

// Per-vertex input (quad vertices)
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;

// Per-instance input
layout(location = 2) in vec3 aInstancePosition;
layout(location = 3) in vec4 aInstanceColor;
layout(location = 4) in float aInstanceSize;

out vec2 vTexCoords;
out vec4 vColor;

layout(location = 0) uniform mat4 u_V;
layout(location = 1) uniform mat4 u_P;

void main() {
    // Billboard quad - scale by instance size
    vec3 worldPos = aInstancePosition + aPos * aInstanceSize;
    
    vTexCoords = aTexCoords;
    vColor = aInstanceColor;
    
    // Calculate final position
    vec4 viewPos = u_V * vec4(worldPos, 1.0);
    gl_Position = u_P * viewPos;
}
