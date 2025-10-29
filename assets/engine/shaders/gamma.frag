#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D tex;

void main() {
    vec3 hdr_color = texture(tex, TexCoords).rgb;

    // simple reinhard tone mapping
    vec3 tone_mapped = hdr_color / (hdr_color + vec3(1.0));
    
    vec3 color = pow(tone_mapped, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}