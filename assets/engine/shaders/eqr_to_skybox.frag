#version 330 core
out vec4 FragColor;
in vec3 WorldPos;

uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
const float MAX_HDR_RADIANCE = 60000.0;

bool IsFiniteVec3(vec3 v) {
    return !(any(isnan(v)) || any(isinf(v)));
}

vec3 SanitizeHdrSample(vec3 v) {
    if (!IsFiniteVec3(v)) {
        return vec3(0.0);
    }
    return clamp(v, vec3(0.0), vec3(MAX_HDR_RADIANCE));
}

vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main() {
    vec2 uv = SampleSphericalMap(normalize(WorldPos));
    vec3 color = SanitizeHdrSample(texture(equirectangularMap, uv).rgb);
    FragColor = vec4(color, 1.0);
}
