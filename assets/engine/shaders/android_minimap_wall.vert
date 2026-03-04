#version 300 es

precision highp float;
precision mediump int;

// Per-vertex: wall triangle position in world XZ space (uploaded once, static)
layout(location = 0) in vec2 aWorldXZ;

// Per-frame uniforms (set once per draw call)
uniform vec2  u_PlayerXZ;       // player position (x, z)
uniform vec2  u_TransformCol0;  // local-space transform column 0
uniform vec2  u_TransformCol1;  // local-space transform column 1
uniform vec2  u_InvDoubleRadius;// per-axis inverse radius scale
uniform vec2  u_NdcBase;        // NDC base offset
uniform vec2  u_NdcScale;       // NDC scale factor

void main() {
    // World-space delta from player
    vec2 delta = aWorldXZ - u_PlayerXZ;

    // Rotate/orient into local minimap space via 2x2 transform
    vec2 local = vec2(
        u_TransformCol0.x * delta.x + u_TransformCol1.x * delta.y,
        u_TransformCol0.y * delta.x + u_TransformCol1.y * delta.y);

    // Map to [0,1] UV (no clamp - glScissor handles clipping)
    float u = 0.5 + local.x * u_InvDoubleRadius.x;
    float v = 0.5 + local.y * u_InvDoubleRadius.y;

    // Convert UV to screen NDC
    vec2 ndc = u_NdcBase + vec2(u, v) * u_NdcScale;
    gl_Position = vec4(ndc, 0.0, 1.0);
}
