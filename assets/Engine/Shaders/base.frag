#version 330 core

in vec3 vColor;
in vec3 vNormal;
in vec3 vFragPos;

out vec4 FragColor;

uniform vec3 u_LightDir;
uniform vec3 u_LightColor;

void main() {
    vec3 norm = normalize(vNormal);
    float diff = max(dot(norm, normalize(-u_LightDir)), 0.0);
    vec3 diffuse = diff * u_LightColor;
    vec3 result = (diffuse + 0.1) * vColor; // +0.1 for ambient
    FragColor = vec4(result, 1.0);
}
