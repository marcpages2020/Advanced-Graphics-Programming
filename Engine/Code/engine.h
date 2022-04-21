//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include <glad/glad.h>

struct aiScene;
struct aiNode;
struct aiMesh;
struct aiMaterial;

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;
typedef glm::mat4  mat4;

struct VertexV3V2
{
	vec3 pos;
	vec2 uv;
};

struct VertexBufferAttribute
{
	u8 location;
	u8 componentCount;
	u8 offset;
};

struct VertexBufferLayout
{
	std::vector<VertexBufferAttribute> attributes;
	u8 stride;
};

struct VertexShaderAttribute
{
	u8 location;
	u8 componentCount;
};

struct VertexShaderLayout
{
	std::vector<VertexShaderAttribute> attributes;
};

mat4 TransformScale(const vec3& scaleFactors);

mat4 TransformPositionScale(const vec3& pos, const vec3& scaleFactors);

struct Vao
{
	GLuint handle;
	GLuint programHandle;
};

struct Submesh
{
	VertexBufferLayout  vertexBufferLayout;
	std::vector<float>  vertices;
	std::vector<u32>    indices;
	u32                 vertexOffset;
	u32                 indexOffset;

	std::vector<Vao>    vaos;
};

struct Mesh
{
	std::vector<Submesh> submeshes;
	GLuint               vertexBufferHandle;
	GLuint               indexBufferHandle;
};

struct Material
{
	std::string name;
	vec3		albedo;
	vec3		emissive;
	f32			smoothness;
	u32			albedoTextureIdx;
	u32			emissiveTextureIdx;
	u32			specularTextureIdx;
	u32			normalsTextureIdx;
	u32			bumpTextureIdx;
};

struct Model
{
	u32              meshIdx;
	std::vector<u32> materialIdx;
};

struct Image
{
	void* pixels;
	ivec2 size;
	i32   nchannels;
	i32   stride;
};

struct Texture
{
	GLuint      handle;
	std::string filepath;
};

struct Program
{
	GLuint             handle;
	std::string        filepath;
	std::string        programName;
	u64                lastWriteTimestamp; // What is this for?
	VertexShaderLayout vertexInputLayout;
};

enum Mode
{
	Mode_TexturedQuad,
	Mode_Count
};

struct Camera
{
	vec3 position;
	vec3 target;
};

struct Entity
{
	mat4 worldMatrix;
	mat4 worldViewProjection;
	vec3 position;
	u32 modelIndex;
	u32 localParamsOffset;
	u32 localParamsSize;
};

struct App
{
	// Loop
	f32  deltaTime;
	bool isRunning;

	// Input
	Input input;

	// Graphics
	char gpuName[64];
	char openGlVersion[64];

	Camera camera;

	ivec2 displaySize;

	// program indices
	u32 texturedMeshProgramIdx;
	u32 texturedGeometryProgramIdx;

	// texture indices
	u32 diceTexIdx;
	u32 whiteTexIdx;
	u32 blackTexIdx;
	u32 normalTexIdx;
	u32 magentaTexIdx;

	// Mode
	Mode mode;

	GLint uniformBlockAlignment;

	// Embedded geometry (in-editor simple meshes such as
	// a screen filling quad, a cube, a sphere...)
	GLuint embeddedVertices;
	GLuint embeddedElements;

	// Location of the texture uniform in the textured quad shader
	GLuint programUniformTexture;
	GLuint texturedMeshProgram_uTexture;

	// VAO object to link our screen filling quad with our textured quad shader
	GLuint VAO;

	u32 model;
	u32 bufferHandle;

	std::vector<Texture> textures;
	std::vector<Material> materials;
	std::vector<Mesh>	 meshes;
	std::vector<Model>	 models;
	std::vector<Program> programs;

	std::vector<Entity> entities;
};

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

void OnGlError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program);

//Assimp
void ProcessAssimpMesh(const aiScene* scene, aiMesh* mesh, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices);
void ProcessAssimpMaterial(App* app, aiMaterial* material, Material& myMaterial, String directory);
void ProcessAssimpNode(const aiScene* scene, aiNode* node, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices);
u32 LoadModel(App* app, const char* filename);

u32 Align(u32 value, u32 alignment);