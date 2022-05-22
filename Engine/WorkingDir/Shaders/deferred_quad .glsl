///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef DEFERRED_QUAD

#if defined(VERTEX) ///////////////////////////////////////////////////

struct Light
{
    unsigned int type;
    vec3 color;
    vec3 direction;
    vec3 position;
};

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

layout(binding = 0, std140) uniform GlobalParams
{
    unsigned int uRenderMode;
    vec3 uCameraPosition;
    unsigned int uLightCount;
    Light uLight[16];
};

layout(binding = 1, std140) uniform LocalParams
{
    mat4 uWorldMatrix;
    mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;

void main()
{
    vTexCoord = aTexCoord;
    gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

struct Light
{
    unsigned int type; //0: Directional, 1 Point
    vec3 color;
    vec3 direction;
    vec3 position;
};

uniform sampler2D uColor;
uniform sampler2D uNormals;
uniform sampler2D uPosition;
uniform sampler2D uDepth;

layout(binding = 0, std140) uniform GlobalParams
{
    unsigned int uRenderMode;
    vec3 uCameraPosition;
    unsigned int uLightCount;
    Light uLight[16];
};

layout(location = 0) out vec4 oColor;

void main()
{
    vec3 vColor    = vec3(texture(uColor, vTexCoord));
    vec3 vNormal   = vec3(texture(uNormals, vTexCoord));
    vec3 vPosition = vec3(texture(uPosition, vTexCoord));

    switch(uRenderMode)
    {
        case 0:
            oColor = vec4(vColor, 1.0f);
            break;
        case 1:
            oColor = vec4(vNormal, 1.0f);
            break;
        case 2:
            oColor = vec4(vPosition, 1.0f);
            break;
        case 3:
            oColor = vec4(vec3(texture(uDepth, vTexCoord).r),1.0);
            break;
        case 4:
           oColor = vec4(vColor, 1.0f);
            break;
        default:
            oColor = vec4(vColor, 1.0f);
            break;
    }
}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.