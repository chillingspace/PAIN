#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D tex;
uniform float threshold;  // typical range [0.8,1.5]

void main() {
    vec3 color = texture(tex, TexCoords).rgb;
    
    // calculate luminance (perceived brightness)
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    
    // keep pixels brighter than threshold.
    // will apply blur on those pixels and then write over
    // original image, in a sense to blur brighter areas
    if (luminance > threshold) {
        FragColor = vec4(color, 1.0);
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}