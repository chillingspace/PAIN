#version 300 es

out vec2 TexCoords;

uniform vec2 pos;
uniform float ndc_scale;    // within range [0, 1]

void main() {
    const vec4 vertices[4] = vec4[](
        vec4(-1.0, -1.0, 0.0, 1.0),
        vec4( 1.0, -1.0, 1.0, 1.0),
        vec4(-1.0,  1.0, 0.0, 0.0),
        vec4( 1.0,  1.0, 1.0, 0.0)
    );
    
    vec4 vertex = vertices[gl_VertexID];
    TexCoords = vertex.zw;
    vec2 fragPos = vertex.xy * ndc_scale + pos;
    gl_Position = vec4(fragPos, 0.0, 1.0);
}