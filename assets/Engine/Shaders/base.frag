// base.frag

#version 330 core

#define PI 3.14159265359

in vec3 vNormal;
in vec3 vFragPos;

out vec4 FragColor;

struct Material {
    float rough;
    float metal;
    vec3 color;
};

struct Light {
    vec3 position;
    vec3 L;         // light intensity
};

uniform Material material;
uniform Light light[1];
uniform mat4 u_M;
uniform mat4 u_V;
uniform mat4 u_P;


float ggxDistribution(float nDotH) {
    float alpha2 = material.rough * material.rough * material.rough * material.rough;
    float d = (nDotH * nDotH) * (alpha2 - 1.0f) + 1.0f;
    return alpha2 / (PI * d * d);
}

float geomSmith(float nDotL) {
    float k = (material.rough + 1.0f) * (material.rough + 1.0f) / 8.0f;
    float denom = nDotL * (1.0f - k) + k;
    return 1.0f / denom;
}

vec3 schlickFresnel(float lDotH) {
    vec3 f0 = vec3(0.04f); // Dielectrics
    if (material.metal == 1.0f)
        f0 = material.color;
    return f0 + (1.0f - f0) * pow(1.0f - lDotH, 5);
}

vec3 microfacetModel(vec3 position, vec3 n) {  
    vec3 diffuseBrdf = material.color;

    vec3 lightI = light[0].L;
    vec3 lightPositionInView = (u_V * vec4(light[0].position, 1.0f)).xyz;

    vec3 l = lightPositionInView - position;
    float dist = length(l);
    l = normalize(l);
    lightI *= 100 / (dist * dist); // Intensity is normalized, so scale up by 100 first

    vec3 v = normalize(-position);
    vec3 h = normalize(v + l);
    float nDotH = dot(n, h);
    float lDotH = dot(l, h);
    float nDotL = max(dot(n, l), 0.0f);
    float nDotV = dot(n, v);
    vec3 specBrdf = 0.25f * ggxDistribution(nDotH) * schlickFresnel(lDotH) 
                            * geomSmith(nDotL) * geomSmith(nDotV);

    return (diffuseBrdf + PI * specBrdf) * lightI * nDotL;
}

void main() {
    vec3 vFragPosViewSpace = (u_V * vec4(vFragPos, 1.0)).xyz;
    vec3 vNormalViewSpace = mat3(u_V) * normalize(vNormal);
    
    vec3 color = microfacetModel(vFragPosViewSpace, vNormalViewSpace);
    FragColor = vec4(color, 1.0);

    // vec3 color = microfacetModel(vFragPos, normalize(vNormal));
    // FragColor = vec4(color, 1.0);
}
