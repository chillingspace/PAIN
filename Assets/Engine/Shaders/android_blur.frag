#version 300 es
precision highp float;

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D tex;
uniform float is_horizontal_pass;
uniform float strength;     // within range [0, inf) - hdr


#define PRECALCULATE_GAUSSIAN

float gaussian(float x, float sigma) {
    return exp(-(x * x) / (2.0 * sigma * sigma)) / (sqrt(2.0 * 3.14159) * sigma);
}


void main() {
    vec2 tex_offset = 1.0 / vec2(textureSize(tex, 0)); // size of single texel
    
#ifdef PRECALCULATE_GAUSSIAN
    const float weight[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
#else
    const int num_weights = 200;  // bigger = blurrier   
    const float sigma = 2.0;
    float weight[200];  // OpenGL ES requires constant array size
    float sum = 0.0;
    for(int i = 0; i < num_weights; ++i) {
        weight[i] = gaussian(float(i), sigma);
        sum += (i == 0) ? weight[i] : 2.0 * weight[i];
    }
    
    // normalize
    for(int i = 0; i < num_weights; ++i) {
        weight[i] /= sum;
    }
#endif
    
    // Start with center pixel
    vec3 result = texture(tex, TexCoords).rgb * weight[0];
    
    if (is_horizontal_pass > 0.5) {
        // Horizontal blur
        for(int i = 1; i < 5; ++i) {
            result += texture(tex, TexCoords + vec2(tex_offset.x * float(i), 0.0)).rgb * weight[i];
            result += texture(tex, TexCoords - vec2(tex_offset.x * float(i), 0.0)).rgb * weight[i];
        }
    }
    else {
        // Vertical blur
        for(int i = 1; i < 5; ++i) {
            result += texture(tex, TexCoords + vec2(0.0, tex_offset.y * float(i))).rgb * weight[i];
            result += texture(tex, TexCoords - vec2(0.0, tex_offset.y * float(i))).rgb * weight[i];
        }
    }
    FragColor = vec4(result * strength, 1.0);

    // FragColor = vec4(1,0,0,1);
}