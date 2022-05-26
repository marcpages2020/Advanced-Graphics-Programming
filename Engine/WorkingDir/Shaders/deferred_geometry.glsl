///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef DEFERRED_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

struct Light
{
    unsigned int type;
    vec3 color;
    vec3 direction;
    vec3 position;
};

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;

layout(binding = 0, std140) uniform GlobalParams
{
    unsigned int uRenderMode;
    vec3         uCameraPosition;
    unsigned int uLightCount;
    Light        uLight[16];
};

layout(binding = 1, std140) uniform LocalParams
{
    mat4 uWorldMatrix;
    mat4 uWorldViewProjectionMatrix;
    float metallic;
    float roughness;
};

out vec2 vTexCoord;
out vec3 vPosition; //In worldspace
out vec3 vNormal;   //In worldspace
out vec3 vViewDir;  //In worldspace
out vec4 FragColor;

uniform mat4 projectionViewMatrix;

void main()
{
    vTexCoord = aTexCoord;
    
    mat4 model = mat4(1.0f);
    //vNormal   = vec3(uWorldMatrix * vec4(aNormal, 0.0));
    vNormal = mat3(transpose(inverse(model))) * aNormal;

    //vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
    vPosition = vec3(model * vec4(aPosition, 1.0f));

    vViewDir  = uCameraPosition - vPosition;

    gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0f);
}   

#elif defined(FRAGMENT) ///////////////////////////////////////////////

struct Light
{
    unsigned int type; //0: Directional, 1 Point
    vec3 color;
    vec3 direction;
    vec3 position;
};

in vec2 vTexCoord;
in vec3 vPosition; //In worldspace
in vec3 vNormal;   //In worldspace
in vec3 vViewDir;  //In worldspace

uniform sampler2D uTexture;
uniform samplerCube cubemap;

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
    float metallic;
    float roughness;
};

layout(location = 0) out vec4 rt0; //Albedo 
layout(location = 1) out vec4 rt1; //Normals 
layout(location = 2) out vec4 rt2; //Position 

void main()
{
    rt0 = texture(uTexture, vTexCoord);
    rt1 = vec4(vNormal, 1.0);
    rt2 = vec4(vPosition, 1.0);

    vec3 I = normalize(vPosition - uCameraPosition);
    vec3 R = reflect(I, normalize(vNormal));

    vec4 rt4 = vec4(texture(cubemap, R).rgb, 1.0f);
    rt0 = mix(rt0, rt4, metallic);
}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.
