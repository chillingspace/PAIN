#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D scene_tex;  // tone-mapped original
uniform sampler2D bloom_tex;  // blurred bright areas
uniform float bloom_strength;

void main() {
    vec3 scene = texture(scene_tex, TexCoords).rgb;
    vec3 bloom = texture(bloom_tex, TexCoords).rgb;
    
    // additive blend
    vec3 result = scene + bloom * bloom_strength;
    
    FragColor = vec4(result, 1.0);
}