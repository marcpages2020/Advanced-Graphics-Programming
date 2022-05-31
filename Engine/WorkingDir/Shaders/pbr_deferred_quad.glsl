///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef DEFERRED_PBR_QUAD

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
out vec3 vWorldPosition;

void main()
{
    vTexCoord = aTexCoord;
    vWorldPosition = aPosition;
    gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vWorldPosition;

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
uniform sampler2D uMetallic;
uniform sampler2D uRoughness;
uniform sampler2D uDepth;

uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

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

layout(location = 0) out vec4 oColor;

const float PI = 3.14159265359;

vec3 CalculateDirectionalLight(Light light, vec3 position, vec3 normal, vec3 viewDir);
vec3 CalculatePointLight(Light light, vec3 position, vec3 normal, vec3 viewDir);
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);

void main()
{
    vec3 vColor      = vec3(texture(uColor, vTexCoord));
    vec3 vNormal     = vec3(texture(uNormals, vTexCoord));
    float alpha      = texture(uNormals,vTexCoord).a;
    vec3 vPosition   = vec3(texture(uPosition, vTexCoord));
    float vMetallic  = vec3(texture(uMetallic, vTexCoord)).r;
    float vRoughness = vec3(texture(uRoughness, vTexCoord)).r;

    oColor = vec4(vec3(0.0f), 1.0f);

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
            oColor = vec4(vec3(vMetallic),1.0);
            break;
        case 5:
            oColor = vec4(vec3(vRoughness),1.0);
            break;
        case 6:
           if(alpha>=0.1)
           {
               vec3 N = vNormal;
               vec3 V = normalize(uCameraPosition - vWorldPosition);
               vec3 R = reflect(-V, N);

               vec3 albedo = vColor.rgb; 

               vec3 F0 = vec3(0.04); 
               F0      = mix(F0, albedo, vMetallic);

               // reflectance equation
               vec3 Lo = vec3(0.0);
               for(int i = 0; i < uLightCount; ++i)
               {
                       vec3 lightDir = normalize(uLight[i].direction);
                       vec3 lightResult = vec3(0.0f);
                       vec3 viewDir = normalize(uCameraPosition - vPosition);

                       vec3 L; 
                       vec3 radiance = uLight[i].color;
                       if(uLight[i].type == 0)
                       {
                           L = uLight[i].direction;
                           
                       }
                       else
                       {
                            L = normalize(uLight[i].position - vWorldPosition);
                            float distance = length(uLight[i].position - vWorldPosition);
                            float attenuation = 1.0 / (distance * distance);
                            radiance *= attenuation;
                       } 

                        vec3 H = normalize(V + L);

                        // Cook-Torrance BRDF
                        float NDF = DistributionGGX(N, H, vRoughness);   
                        float G   = GeometrySmith(N, V, L, vRoughness);    
                        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       

                        vec3 numerator    = NDF * G * F;
                        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0)  + 0.0001;
                        vec3 specular     = numerator / denominator; 

                        vec3 kS = F;
                        vec3 kD = vec3(1.0) - kS;
                        kD *= 1.0 - vMetallic;     

                        // add to outgoing radiance Lo
                        float NdotL = max(dot(N, L), 0.0);                
                        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
                        
                       //oColor.rgb += lightResult * albedo.rgb;    
               }

                vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, vRoughness);
               vec3 kS = F;
               vec3 kD = 1.0 - kS;
               kD *= 1.0 - vMetallic;

               vec3 irradiance = texture(irradianceMap, N).rgb;
               vec3 diffuse = irradiance * albedo;

               const float MAX_REFLECTION_LOD = 4.0;
               vec3 prefilteredColor = textureLod(prefilterMap, R,  vRoughness * MAX_REFLECTION_LOD).rgb;  
               vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), vRoughness)).rg;
               vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

               vec3 ambient = kD * diffuse + specular;
               
               vec3 color = ambient + Lo;

                color = color / (color + vec3(1.0f));
                color = pow(color, vec3(1.0f/2.2f));

                oColor = vec4(color, 1.0f);
           }
           else{
               oColor = vec4(vColor, 1.0f);
           }
            break;
        default:;
            oColor = vec4(vColor, 1.0f);
            break;
    }
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.