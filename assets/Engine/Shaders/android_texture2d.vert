#version 300 es

out vec2 TexCoords;

uniform vec2 pos;
uniform float ndc_scale;    // within range [0, 1]

void main() {
    const vec2 vertices[4] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2(-1.0,  1.0),
        vec2( 1.0,  1.0)
    );
    vec2 vertex = vertices[gl_VertexID];
    TexCoords = vertex * 0.5 + 0.5;
    vec2 fragPos = vertex * ndc_scale + pos;
    gl_Position = vec4(fragPos, 0.0, 1.0);
}