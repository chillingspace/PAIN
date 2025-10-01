// floor.vert


#version 330 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_uniform_location : enable


layout(location=0) out vec3 fFragPos;

uniform mat4 u_M;       // light only 
uniform mat4 u_V;
uniform mat4 u_P;

uniform float u_ShadowPass;


void main() {
    const vec2 positions[4] = vec2[4](
        vec2(-1, -1),
        vec2(1, -1),
        vec2(1, 1),
        vec2(-1, 1)
    );

    vec2 pos = positions[gl_VertexID];

    // create scale mtx
    const float floor_size = 1000.0;
    const mat4 scale = mat4(
        floor_size, 0, 0, 0,
        0, floor_size, 0, 0,
        0, 0, floor_size, 0,
        0, 0, 0, 1
    );

    // create rot mtx to rotate floor quad 90 degrees along x axis
    float angle = radians(90.0);
    float c = cos(angle);
    float s = sin(angle);

    mat4 rot = mat4(
        1, 0, 0, 0,
        0, c, s, 0,
        0, -s, c, 0,
        0, 0, 0, 1
    );

    mat4 trans = mat4(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        -floor_size/2.0, 0, -floor_size/2.0, 1
    );

    // translate not required(its a floor)

    mat4 xform = trans * rot * scale;

    vec4 world_pos = xform * vec4(pos, 0, 1);

    if (u_ShadowPass > 0.0) {
        gl_Position = u_P * u_V * u_M * world_pos;
        return;
    }

    fFragPos = world_pos.xyz;

    gl_Position = u_P * u_V * world_pos;
}
