// floor.vert


#version 330 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_uniform_location : enable


layout(location=0) out vec3 fFragPos;


layout(location=0) uniform mat4 u_M;
layout(location=1) uniform mat4 u_V;
layout(location=2) uniform mat4 u_P;


void main() {
    const vec2 positions[4] = vec2[](
        vec2(-1, -1),
        vec2(1, -1),
        vec2(1, 1),
        vec2(-1, 1),
    );

    const vec2 pos = positions[gl_VertexID];

    // create scale mtx
    const float floor_size = 1000.0;
    const mat4 scale = mat4(
        floor_size, 0, 0, 0,
        0, floor_size, 0, 0,
        0, 0, floor_size, 0,
        0, 0, 0, floor_size
    );

    // create rot mtx to rotate floor quad 90 degrees along x axis
    const float angle = radians(90);
    const float c = cos(angle);
    const float s = sin(angle);

    const mat4 rot = mat4(
        1, 0, 0, 0,
        0, c, s, 0,
        0, -s, c, 0,
        0, 0, 0, 1
    );

    // translate not required(its a floor)

    const mat4 xform = rot * scale;

    const vec4 world_pos = xform * vec4(pos, 0, 1);

    fFragPos = world_pos.xyz;

    gl_Position = u_P * u_V * world_pos;
}
