#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D tex;
uniform float exposure;
uniform float toneMapMode;
uniform float u_gamma;

// ACES - Stephen Hill's approximation, outputs linear LDR (no gamma bake)
vec3 ACESFilm(vec3 color) {
    color = mat3(0.59719, 0.07600, 0.02840,
                 0.35458, 0.90834, 0.13383,
                 0.04823, 0.01566, 0.83777) * color;
    vec3 a = color * (color + 0.0245786) - 0.000090537;
    vec3 b = color * (0.983729 * color + 0.4329510) + 0.238081;
    color = a / b;
    color = mat3( 1.60475, -0.10208, -0.00327,
                 -0.53108,  1.10813, -0.07276,
                 -0.07367, -0.00605,  1.07602) * color;
    return clamp(color, 0.0, 1.0);
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
    else {
        mapped = vec3(1, 0, 1);
    }

    // Apply gamma correction in same pass
    mapped = pow(mapped, vec3(1.0 / u_gamma));

    FragColor = vec4(mapped, 1.0);
}
