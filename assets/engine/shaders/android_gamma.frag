#version 300 es

precision highp float;

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D tex;
uniform float u_gamma;

void main() {
    vec3 hdr_color = texture(tex, TexCoords).rgb;

    // simple reinhard tone mapping
    // vec3 tone_mapped = hdr_color / (hdr_color + vec3(1.0));
    
    vec3 color = pow(hdr_color, vec3(1.0 / u_gamma));
    
    FragColor = vec4(color, 1.0);
}