#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D tex;
uniform float exposure;
uniform float toneMapMode;

// realistic
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

// cinematic
vec3 Uncharted2Tonemap(vec3 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 Uncharted2(vec3 color) {
    float W = 11.2;
    vec3 curr = Uncharted2Tonemap(color);
    vec3 whiteScale = 1.0 / Uncharted2Tonemap(vec3(W));
    return curr * whiteScale;
}

void main() {
    vec3 hdr_color = texture(tex, TexCoords).rgb;
    hdr_color = min(hdr_color, vec3(65000.0));
    // const float exposure = 1.0;
    
    vec3 x = hdr_color * exposure;
    vec3 mapped;

    if (toneMapMode == 0.0) {
        // none
        mapped = clamp(x, 0.0, 1.0);
    }
    else if (toneMapMode == 1.0) {
        mapped = ACESFilm(x);
    }
    else if (toneMapMode == 2.0) {
        // reinhard
        mapped = x / (x + vec3(1.0));
    }
    else if (toneMapMode == 3.0) {
        mapped = Uncharted2(x);
    }

    FragColor = vec4(mapped, 1.0);
}