// @file	base.vert
// @brief	base vertex shader
// @author	Poh Jing Seng, 2301363, jingseng.poh@digipen.edu (10%)
// @co-author Sean Gwee, 2301326, g.boonxuensean@digipen.edu (90%)
// @date	11 Sep 2024

#version 450 core

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec3 a_color;

layout (location = 0) out vec4 f_color;

layout(location = 0) uniform mat4 model_to_ndc;

void main() {
    // Correct way: let the matrix do the full transformation
    gl_Position = model_to_ndc * vec4(a_position, 0.0, 1.0);
    f_color = vec4(a_color, 1.0);
}

