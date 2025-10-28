#version 330 core

out vec2 TexCoords;

void main() {
    const vec2 vertices[4] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2(-1.0,  1.0),
        vec2( 1.0,  1.0)
    );
    
    vec2 vertex = vertices[gl_VertexID];
    TexCoords = vertex * 0.5 + 0.5;
    gl_Position = vec4(vertex, 0.0, 1.0);
}