#version 300 es
precision mediump float;

out vec4 FragColor;

in vec2 TexCoord;
in vec2 ScreenPos;

uniform float animationTime;
uniform vec3 color1;  // Top color
uniform vec3 color2;  // Bottom color
uniform float overlayStrength;  // Overlay intensity (0.0 to 1.0)

// Simple noise function
float noise(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    // Animated vertical gradient
    float gradientFactor = TexCoord.y + sin(animationTime * 0.5 + TexCoord.x * 3.0) * 0.05;
    vec3 gradient = mix(color2, color1, gradientFactor);
    
    // Subtle noise overlay
    float n = noise(ScreenPos * 1000.0 + animationTime) * 0.02;
    
    // Radial darkening from center
    float dist = length(ScreenPos);
    float vignette = 1.0 - smoothstep(0.5, 1.5, dist);
    
    vec3 finalColor = gradient * (1.0 + n) * (0.6 + vignette * 0.4);
    FragColor = vec4(finalColor, overlayStrength);
}
