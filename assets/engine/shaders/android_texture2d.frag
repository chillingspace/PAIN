#version 300 es
precision highp float;
precision highp sampler2D;

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D tex;

void main() {
    FragColor = texture(tex, TexCoords);
}