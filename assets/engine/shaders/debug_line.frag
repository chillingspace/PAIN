#version 330 core
out vec4 FragColor; // Output fragment color

uniform vec3 u_Color; // Color passed from C++

void main()
{
    // Set fragment color to the uniform color with full alpha
    FragColor = vec4(u_Color, 1.0);
}