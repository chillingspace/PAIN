#version 330 core

out vec2 TexCoords;

void main() {
    const vec4 vertices[4] = vec4[](
        vec4(-1.0, -1.0, 0.0, 0.0),
        vec4( 1.0, -1.0, 1.0, 0.0),
        vec4(-1.0,  1.0, 0.0, 1.0),
        vec4( 1.0,  1.0, 1.0, 1.0)
    );
    
    vec4 vertex = vertices[gl_VertexID];
    TexCoords = vertex.zw;
    gl_Position = vec4(vertex.xy, 0.0, 1.0);
}
