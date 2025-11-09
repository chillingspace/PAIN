#version 330 core
out vec4 FragColor;
in vec3 WorldPos;

uniform samplerCube environmentMap;

const float PI = 3.14159265359;

void main()
{		

// #define DEBUG
#ifdef DEBUG
    vec3 color = texture(environmentMap, normalize(WorldPos)).rgb;
    FragColor = vec4(color, 1.0);
    return;
#endif

// #define DEBUG_ENVMAP
#ifdef DEBUG_ENVMAP
    vec4 color = vec4(texture(environmentMap, normalize(WorldPos)).rgb, 1.0);
    if (color == vec4(0,0,0,1)) FragColor = vec4(1,0,1,1);
    else FragColor = vec4(texture(environmentMap, normalize(WorldPos)).rgb, 1.0);
    return;
#endif

// if solid color/gradient, all good.
// #define DEBUG_POS
#ifdef DEBUG_POS
    vec3 dir = normalize(WorldPos);
    FragColor = vec4(abs(dir), 1.0); // visualize directions
    return;
#endif


    // The world vector acts as the normal of a tangent surface
    // from the origin, aligned to WorldPos. Given this normal, calculate all
    // incoming radiance of the environment. The result of this radiance
    // is the radiance of light coming from -Normal direction, which is what
    // we use in the PBR shader to sample irradiance.
    vec3 N = normalize(WorldPos);

    vec3 irradiance = vec3(0.0);   
    
    // Tangent space calculation from origin point
    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up         = normalize(cross(N, right));
       
    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    
    // Convolve environment map by sampling hemisphere around normal
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            // Spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // Tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N; 

            if (any(isnan(N)) || any(isnan(sampleVec))) {
                FragColor = vec4(1.0, 0.0, 1.0, 1.0); // magenta = invalid vector
                return;
            }

            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples += cos(theta) * sin(theta);

            nrSamples++;
        }
    }

    // DEBUG: Check for div by zero
    if (nrSamples < 1.0) {
        FragColor = vec4(1.0, 0.0, 1.0, 1.0); // Magenta = error
        return;
    }
    
    irradiance = PI * irradiance * (1.0 / float(nrSamples));
    // irradiance = PI * irradiance / nrSamples;
    
    // Safety clamp
    // irradiance = min(irradiance, vec3(100.0));
    
    irradiance = min(irradiance, vec3(1000.0));
    FragColor = vec4(irradiance, 1.0);
}