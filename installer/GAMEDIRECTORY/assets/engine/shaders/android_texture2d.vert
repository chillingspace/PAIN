#version 300 es

out vec2 TexCoords;

uniform vec2 pos;
uniform vec2 ndc_scale; 

void main() {
    // Use same vertex data as Windows for consistency
    const vec2 vertices[4] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2(-1.0,  1.0),
        vec2( 1.0,  1.0)
    );

    vec2 vertex = vertices[gl_VertexID];
    
    // vertically flipped
    TexCoords = vec2(vertex.x * 0.5 + 0.5, 1.0 - (vertex.y * 0.5 + 0.5));
    
    //uses vec2 ndc_scale
    vec2 fragPos = vertex * ndc_scale + pos;
    
    gl_Position = vec4(fragPos, 0.0, 1.0);
}
