#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D tex;
uniform int u_ClipCircle;      // 0 = no clip, 1 = circular clip
uniform float u_Opacity;

void main() {
    // Always test the clipping logic for debugging
    vec2 uvCenter = vec2(0.5, 0.5);
    float dist = length(TexCoords - uvCenter);
    
    if (u_ClipCircle > 0 && dist > 0.5) {
        discard;
    }
    
    FragColor = texture(tex, TexCoords) * vec4(1.0, 1.0, 1.0, u_Opacity);
}