// particle.frag

#version 330 core

in vec2 vTexCoords;
in vec4 vColor;

out vec4 FragColor;

uniform sampler2D tex;

void main() {
    // Sample texture
    vec4 texColor = texture(tex, vTexCoords);
    
    // Apply vertex color (tint)
    FragColor = texColor * vColor;
    
    // Discard fully transparent pixels
    if (FragColor.a < 0.01) {
        discard;
    }
}
