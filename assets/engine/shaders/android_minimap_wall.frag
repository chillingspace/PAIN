#version 300 es

#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
precision mediump int;

uniform vec4 u_Color;
out vec4 FragColor;

void main() {
    FragColor = u_Color;
}
