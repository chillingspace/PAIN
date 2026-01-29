#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec2 ScreenPos;

uniform float progress;        // 0.0 to 1.0
uniform float animationTime;   // For shimmer effect
uniform vec3 fillColor;        // Main progress color
uniform vec3 glowColor;        // Glow/edge color
uniform float glowIntensity;   // Glow strength

void main() {
    // Check if this fragment is in the filled portion
    float fillAmount = progress;
    
    if (TexCoord.x <= fillAmount) {
        // Inside filled portion
        
        // Shimmer effect
        float shimmer = sin(TexCoord.x * 10.0 - animationTime * 2.0) * 0.1 + 0.9;
        
        // Gradient from bottom to top
        vec3 gradientColor = mix(fillColor * 0.7, fillColor, TexCoord.y);
        
        // Edge glow near progress edge
        float distToEdge = abs(TexCoord.x - fillAmount);
        float glow = exp(-distToEdge * 50.0) * glowIntensity;
        
        vec3 finalColor = gradientColor * shimmer + glowColor * glow;
        FragColor = vec4(finalColor, 1.0);
    } else {
        // Empty portion (background)
        vec3 bgColor = vec3(0.2, 0.2, 0.2);
        FragColor = vec4(bgColor, 1.0);
    }
}
