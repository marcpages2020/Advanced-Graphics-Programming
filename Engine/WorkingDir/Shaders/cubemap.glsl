///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef CUBEMAP

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;

uniform mat4 projection;
uniform mat4 view;

out vec3 vTexCoord;

void main()
{
    vTexCoord = aPosition;
    vec4 pos = projection * view * vec4(aPosition, 1.0f);
    gl_Position = pos.xyww;
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec3 vTexCoord;
uniform samplerCube cubemap;

layout(location = 0) out vec4 oColor;
layout(location=1) out vec4 normal;

void main()
{
    oColor = texture(cubemap, vTexCoord);
    normal = vec4(0.0);
}

#endif
#endif