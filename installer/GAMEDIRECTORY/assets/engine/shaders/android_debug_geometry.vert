#version 300 es

precision highp float;
precision mediump int;

layout(location=0) in vec3 aPos;
layout(location=1) in vec4 aCol;

uniform mat4 u_V, u_P;
out vec4 vCol;

void main(){ 
    vCol = aCol; 
    gl_Position = u_P * u_V * vec4(aPos,1.0);
 }