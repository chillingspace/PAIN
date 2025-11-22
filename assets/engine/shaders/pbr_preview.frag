#version 330 core

#define PI 3.14159265359

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

// Material properties
uniform vec3 u_BaseColor;
uniform float u_Metallic;
uniform float u_Roughness;

// Texture maps
uniform sampler2D u_AlbedoMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_MetallicMap;
uniform sampler2D u_RoughnessMap;
uniform sampler2D u_AOMap;
uniform sampler2D u_EmissiveMap;

// Texture usage flags
uniform bool u_UseAlbedoMap;
uniform bool u_UseNormalMap;
uniform bool u_UseMetallicMap;
uniform bool u_UseRoughnessMap;
uniform bool u_UseAOMap;
uniform bool u_UseEmissiveMap;

// Lighting
uniform vec3 u_CamPos;
uniform vec3 u_LightPos;
uniform vec3 u_LightColor;
uniform vec3 u_AmbientLight;

// PBR functions (keep your existing ones)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Unpack normal map
vec3 getNormalFromMap() {
    vec3 tangentNormal = texture(u_NormalMap, TexCoords).xyz * 2.0 - 1.0;
    
    // Simple normal mapping without tangent space
    // For proper implementation, pass tangent/bitangent from vertex shader
    return normalize(Normal);  // Fallback for now
}

void main() {
    // Sample textures
    vec3 albedo = u_UseAlbedoMap ? 
        texture(u_AlbedoMap, TexCoords).rgb * u_BaseColor : 
        u_BaseColor;
    
    float metallic = u_UseMetallicMap ? 
        texture(u_MetallicMap, TexCoords).r : 
        u_Metallic;
    
    float roughness = u_UseRoughnessMap ? 
        texture(u_RoughnessMap, TexCoords).r : 
        u_Roughness;
    
    float ao = u_UseAOMap ? 
        texture(u_AOMap, TexCoords).r : 
        1.0;
    
    vec3 emissive = u_UseEmissiveMap ? 
        texture(u_EmissiveMap, TexCoords).rgb : 
        vec3(0.0);
    
    // Normal
    vec3 N = u_UseNormalMap ? getNormalFromMap() : normalize(Normal);
    vec3 V = normalize(u_CamPos - FragPos);
    
    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // Lighting calculation
    vec3 L = normalize(u_LightPos - FragPos);
    vec3 H = normalize(V + L);
    float distance = length(u_LightPos - FragPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = u_LightColor * attenuation;
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
    
    // Ambient with AO
    vec3 ambient = u_AmbientLight * albedo * ao;
    
    vec3 color = ambient + Lo + emissive;
    
    FragColor = vec4(color, 1.0);
}
