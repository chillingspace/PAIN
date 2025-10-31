#version 300 es

#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;   // prefer highp if available
#else
precision mediump float; // fallback if device lacks highp in FS
#endif
precision mediump int;

in vec4 vCol;
out vec4 FragColor;

void main(){
     FragColor = vCol; 
}