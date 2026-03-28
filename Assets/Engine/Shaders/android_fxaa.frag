#version 300 es
precision highp float;

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D tex;
uniform vec2 u_texel_size;

float luma(vec3 rgb) {
    return dot(rgb, vec3(0.299, 0.587, 0.114));
}

void main() {
    vec2 uv = TexCoords;

    vec3 rgbNW = texture(tex, uv + vec2(-1.0, -1.0) * u_texel_size).rgb;
    vec3 rgbNE = texture(tex, uv + vec2( 1.0, -1.0) * u_texel_size).rgb;
    vec3 rgbSW = texture(tex, uv + vec2(-1.0,  1.0) * u_texel_size).rgb;
    vec3 rgbSE = texture(tex, uv + vec2( 1.0,  1.0) * u_texel_size).rgb;
    vec3 rgbM  = texture(tex, uv).rgb;

    float lumaNW = luma(rgbNW);
    float lumaNE = luma(rgbNE);
    float lumaSW = luma(rgbSW);
    float lumaSE = luma(rgbSE);
    float lumaM  = luma(rgbM);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    float lumaRange = lumaMax - lumaMin;

    // Skip pixels that are not on an edge
    if (lumaRange < max(0.0312, lumaMax * 0.125)) {
        FragColor = vec4(rgbM, 1.0);
        return;
    }

    // Determine blend direction along the edge
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.03125, 0.0078125);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, vec2(-8.0), vec2(8.0)) * u_texel_size;

    // Two-tap blend: inner samples
    vec3 rgbA = 0.5 * (
        texture(tex, uv + dir * (1.0 / 3.0 - 0.5)).rgb +
        texture(tex, uv + dir * (2.0 / 3.0 - 0.5)).rgb);

    // Four-tap blend: add outer samples
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(tex, uv + dir * -0.5).rgb +
        texture(tex, uv + dir *  0.5).rgb);

    float lumaB = luma(rgbB);
    if (lumaB < lumaMin || lumaB > lumaMax) {
        FragColor = vec4(rgbA, 1.0);
    } else {
        FragColor = vec4(rgbB, 1.0);
    }
}
