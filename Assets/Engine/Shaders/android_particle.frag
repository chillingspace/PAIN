// android_particle.frag

#version 300 es
precision highp float;

in vec2 vTexCoords;
in vec4 vColor;

out vec4 FragColor;

uniform sampler2D tex;
uniform int u_UseTexture;
uniform int u_Shape;
uniform float u_SoftEdge;

void main() {
    vec2 centerUV = vTexCoords * 2.0 - 1.0;
    float dist = length(centerUV);

    if (u_Shape == 1 && dist > 1.0) {
        discard;
    }

    float shapeAlpha = 1.0;
    if (u_Shape == 2) {
        if (dist > 1.0) {
            discard;
        }
        float edge = clamp(u_SoftEdge, 0.0001, 1.0);
        shapeAlpha = 1.0 - smoothstep(1.0 - edge, 1.0, dist);
    }

    vec4 texColor = (u_UseTexture == 1) ? texture(tex, vTexCoords) : vec4(1.0);
    
    // Apply vertex color (tint)
    FragColor = texColor * vColor;
    FragColor.a *= shapeAlpha;
    
    // Discard fully transparent pixels
    if (FragColor.a < 0.01) {
        discard;
    }
}
