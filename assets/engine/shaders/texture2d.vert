#version 330 core
out vec2 TexCoords;

uniform vec2 pos;
uniform vec2 ndc_scale;    // within  range [0, 1]
uniform vec4 uv_transform; // x=scaleU, y=scaleV, z=offsetU, w=offsetV

void main() {
    const vec2 vertices[4] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2(-1.0,  1.0),
        vec2( 1.0,  1.0)
    );
    vec2 vertex = vertices[gl_VertexID];
    
    // Base 0-1 UVs (with vertical flip if needed by engine convention)
    vec2 baseUV = vec2(vertex.x * 0.5 + 0.5, 1.0 - (vertex.y * 0.5 + 0.5));
    
    // Apply spritesheet transform: UV = base * scale + offset
    TexCoords = baseUV * uv_transform.xy + uv_transform.zw;
    
    vec2 fragPos = vertex * ndc_scale + pos;
    gl_Position = vec4(fragPos, 0.0, 1.0);
}