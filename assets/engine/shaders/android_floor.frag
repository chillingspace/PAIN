#version 300 es
precision highp float;

in vec3 fFragPos;

layout(location = 0) out vec3 gPos;
layout(location = 1) out vec3 gCol;
layout(location = 2) out vec3 gNorm;
layout(location = 3) out vec3 gMaterial;


void main() {
    const float tile_size = 1.0;

    vec2 tile_coords = floor(fFragPos.xz / tile_size);

    float dark_tile_intensity = mod(tile_coords.x + tile_coords.y, 2.0);

    const vec3 dark = vec3(0.05);
    const vec3 light = vec3(0.5);

    const float roughness = 1.0;        // 1 -> rough, 0.0 -> smooth
    const float metallic = 0.0;         // non-metal

    gPos = fFragPos;
    gCol = mix(light, dark, dark_tile_intensity);
    gNorm = vec3(0.0, 1.0, 0.0);
    gMaterial = vec3(roughness, metallic, 0.0);
}