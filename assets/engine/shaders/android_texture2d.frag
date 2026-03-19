#version 300 es
precision highp float;
precision highp sampler2D;

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D tex;
uniform int u_ClipCircle;      // 0 = no clip, 1 = circular clip

void main() {
    vec2 uvCenter = vec2(0.5, 0.5);
    float dist = length(TexCoords - uvCenter);
    
    if (u_ClipCircle > 0 && dist > 0.5) {
        discard;
    }
    
    FragColor = texture(tex, TexCoords);
}