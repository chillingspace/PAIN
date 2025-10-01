#version 300 es
precision highp float;

#define PI 3.14159265359

in vec2 TexCoords;
layout(location = 0) out vec4 FragColor;

struct Material {
    float rough;
    float metal;
    vec3 color;
};

struct Light {
    vec3 position;
    vec3 L;         // light intensity
};

#define MAX_LIGHTS 16
uniform Light u_Lights[MAX_LIGHTS];
uniform float u_NumLights;
uniform vec3 u_AmbientLight;

uniform mat4 u_M;
uniform mat4 u_V;
uniform mat4 u_P;

uniform sampler2D gPos;
uniform sampler2D gCol;
uniform sampler2D gNorm;
uniform sampler2D gMaterial;

Material material;


float ggxDistribution(float nDotH) {
    float alpha2 = material.rough * material.rough * material.rough * material.rough;
    float d = (nDotH * nDotH) * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * d * d);
}

float geomSmith(float nDotL) {
    float k = (material.rough + 1.0) * (material.rough + 1.0) / 8.0;
    float denom = nDotL * (1.0 - k) + k;
    return 1.0 / denom;
}

vec3 schlickFresnel(float lDotH) {
    vec3 f0 = vec3(0.04); // Dielectrics
    if (material.metal == 1.0)
        f0 = material.color;
    return f0 + (1.0 - f0) * pow(1.0 - lDotH, 5.0);
}

vec3 microfacetModel(vec3 position, vec3 n, Light light) {  
    vec3 diffuseBrdf = material.color;

    vec3 lightI = light.L;
    vec3 lightPositionInView = (u_V * vec4(light.position, 1.0)).xyz;

    vec3 l = lightPositionInView - position;
    float dist = length(l);
    l = normalize(l);
    lightI *= 100.0 / (dist * dist + 100.0); // Intensity is normalized, so scale up by 100 first

    vec3 v = normalize(-position);
    vec3 h = normalize(v + l);
    float nDotH = dot(n, h);
    float lDotH = dot(l, h);
    float nDotL = max(dot(n, l), 0.0);
    float nDotV = dot(n, v);
    vec3 specBrdf = 0.25 * ggxDistribution(nDotH) * schlickFresnel(lDotH) 
                            * geomSmith(nDotL) * geomSmith(nDotV);

    return (diffuseBrdf + PI * specBrdf) * lightI * nDotL;
}

void main() {
    vec3 fragPos = texture(gPos, TexCoords).rgb;
    material.color = texture(gCol, TexCoords).rgb;
    vec3 normal = texture(gNorm, TexCoords).rgb;
    vec2 m = texture(gMaterial, TexCoords).rg;

    material.rough = m.r;
    material.metal = m.g;

    vec3 viewFragPos = (u_V * vec4(fragPos, 1.0)).xyz;
    vec3 viewNormal = mat3(u_V) * normalize(normal);

    vec3 color = material.color * u_AmbientLight;
    for (int i=0; i < int(u_NumLights); i++) {
        color += microfacetModel(viewFragPos, viewNormal, u_Lights[i]);
    }
    FragColor = vec4(color, 1.0);
}