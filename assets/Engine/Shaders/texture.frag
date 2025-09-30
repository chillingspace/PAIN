#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D tex;

void main() {
    vec3 color = texture(tex, TexCoords).rgb;
    
    // Debug: if texture is empty, show UV coordinates as color
    if (length(color) < 0.01) {
        FragColor = vec4(TexCoords, 0.0, 1.0);  // Red-green gradient if empty
    } else {
        FragColor = vec4(color, 1.0);
    }
}