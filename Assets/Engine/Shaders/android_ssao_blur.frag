#version 300 es
precision highp float;
precision highp sampler2D;

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D u_Ssao;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(u_Ssao, 0));
    float result = 0.0;
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            result += texture(u_Ssao, TexCoords + vec2(float(x), float(y)) * texelSize).r;
        }
    }
    result /= 25.0;
    FragColor = vec4(vec3(result), 1.0);
}
