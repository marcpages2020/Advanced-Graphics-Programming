///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef IRRADIANCE_MAP

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;

uniform mat4 projection;
uniform mat4 view;

out vec3 worldPos;

void main()
{
    worldPos = aPosition;
    gl_Position = projection * view * vec4(worldPos, 1.0f);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec3 WorldPos;

uniform samplerCube environmentMap;

layout(location = 0) out vec4 oColor;

const float PI = 3.14159265359;

void main()
{
    vec3 N = normalize(WorldPos);
    vec3 irradiance = vec3(0.0);   

    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up         = normalize(cross(N, right));

    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N; 

            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    
    irradiance = PI * irradiance * (1.0 / float(nrSamples));

    oColor = vec4(irradiance, 1.0);
    oColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}

#endif
#endif