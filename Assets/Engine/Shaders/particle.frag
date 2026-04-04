// particle.frag

#version 330 core

in vec2 vTexCoords;
in vec4 vColor;

out vec4 FragColor;

uniform sampler2D tex;
uniform sampler2D u_SceneDepth;
uniform int u_UseTexture;
uniform int u_Shape;
uniform float u_SoftEdge;
uniform vec2 u_InvScreenSize;
uniform float u_DepthFadeDistance;
uniform vec2 u_NearFar;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * u_NearFar.x * u_NearFar.y) / (u_NearFar.y + u_NearFar.x - z * (u_NearFar.y - u_NearFar.x));
}

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

    vec2 screenUv = gl_FragCoord.xy * u_InvScreenSize;
    float sceneDepth = texture(u_SceneDepth, screenUv).r;
    float linearSceneDepth = LinearizeDepth(sceneDepth);
    float linearParticleDepth = LinearizeDepth(gl_FragCoord.z);
    float depthDelta = max(0.0, linearSceneDepth - linearParticleDepth);
    float depthFade = clamp(depthDelta / max(0.0001, u_DepthFadeDistance), 0.0, 1.0);
    FragColor.a *= depthFade;
    
    // Discard fully transparent pixels
    if (FragColor.a < 0.01) {
        discard;
    }
}
