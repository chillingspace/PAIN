// @file	base.frag
// @brief	base fragment shader
// @author	Poh Jing Seng, 2301363, jingseng.poh@digipen.edu (10%)
// @co-author Sean Gwee, 2301326, g.boonxuensean@digipen.edu (90%)
// @date	11 Sep 2024

#version 450 core

// layout (location=0) in vec3 f_color; (uses uniform now)

layout (location=0) in vec4 a_color;
uniform vec3 f_color;
uniform float f_opacity;
uniform int override_color;

layout (location=0) out vec4 out_color;

void main() {
	// now batch rendering is being commented out for now
	if (override_color == 1) {
		out_color = vec4(f_color, f_opacity);
	} else {
		// out_color = vec4(a_color, 1);
		out_color = vec4(f_color, f_opacity);
	}
}

