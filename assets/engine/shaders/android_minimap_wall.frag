#version 300 es

#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
precision mediump int;

uniform vec4 u_Color;
uniform vec4 u_AccentColor;
uniform float u_PatternStrength;
uniform float u_PatternScale;
uniform float u_PatternPhase;
in vec2 vMapUv;
out vec4 FragColor;

void main() {
    float bandCoord = (vMapUv.x + vMapUv.y) * u_PatternScale - u_PatternPhase;
    float band = smoothstep(0.18, 0.48, abs(fract(bandCoord) - 0.5));
    vec4 color = mix(u_Color, u_AccentColor, band * clamp(u_PatternStrength, 0.0, 1.0));
    FragColor = color;
}
