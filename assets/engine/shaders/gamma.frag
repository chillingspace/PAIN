#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D tex;

void main() {
    vec3 color = texture(tex, TexCoords).rgb;
    
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}