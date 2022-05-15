///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef SHOW_COMBINED

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
    vec3         uCameraPosition;
    unsigned int uLightCount;
    Light        uLight[16];
};

layout(binding = 1, std140) uniform LocalParams
{
    mat4 uWorldMatrix;
    mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPosition; //In worldspace
out vec3 vNormal;   //In worldspace
out vec3 vViewDir;  //In worldspace

uniform mat4 projectionViewMatrix;

void main()
{
    vTexCoord = aTexCoord;
    vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
    vNormal   = vec3(uWorldMatrix * vec4(aNormal, 0.0)); 
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

layout(binding = 0, std140) uniform GlobalParams
{
    vec3 uCameraPosition;
    unsigned int uLightCount;
    Light uLight[16];
};

layout(location = 0) out vec4 rt0; //Albedo 
layout(location = 1) out vec4 rt1; //Normals 
layout(location = 2) out vec4 rt2; //Position 
layout(location = 3) out vec4 rt3; //Final Render 

vec3 CalculateDirectionalLight(Light light, vec3 normal, vec3 viewDir);
vec3 CalculatePointLight(Light light, vec3 normal, vec3 viewDir);

void main()
{
    rt0 = texture(uTexture, vTexCoord);
    rt1 = vec4(vNormal, 1.0);
    rt2 = vec4(vPosition, 1.0);
    rt3 = vec4(vec3(0.0), 1.0);

    for(int i = 0; i < uLightCount; ++i)
    {
        vec3 lightDir = normalize(uLight[i].direction);
        vec3 lightResult = vec3(0.0f);

        if(uLight[i].type == 0)
        {
            lightResult = CalculateDirectionalLight(uLight[i], normalize(vNormal), vViewDir);
        }
        else
        {
            lightResult = CalculatePointLight(uLight[i], normalize(vNormal), vViewDir);
        }
        
        rt3.rgb += lightResult * rt0.rgb;    
    }
}

vec3 CalculateDirectionalLight(Light light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(light.direction);

    float ambientStrenght = 0.2;
    vec3  ambient = ambientStrenght * light.color;

    float diff = max(dot(normal, lightDir), 0.0f);
    vec3 diffuse = diff * light.color;

    float specularStrength = 0.1f;
    vec3 reflectDir = reflect(-lightDir, vNormal);
    
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 2);
    vec3 specular = specularStrength * spec * light.color;

    return ambient + diffuse + specular;
}

vec3 CalculatePointLight(Light light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = vPosition - light.position;
    lightDir = normalize(-lightDir);

    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;

    float distance = length(light.position - vPosition);
    float attenuation = 1.0f / (constant + linear * distance + quadratic * (distance * distance));

    float ambientStrenght = 0.2;
    vec3  ambient = ambientStrenght * light.color;

    float diff = max(dot(normal, lightDir), 0.0f);
    vec3 diffuse = diff * light.color;

    float specularStrength = 0.1f;
    vec3 reflectDir = reflect(-lightDir, vNormal);
    
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 2);
    vec3 specular = specularStrength * spec * light.color;

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.
