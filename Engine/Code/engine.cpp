//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
	GLchar  infoLogBuffer[1024] = {};
	GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
	GLsizei infoLogSize;
	GLint   success;

	char versionString[] = "#version 430\n";
	char shaderNameDefine[128];
	sprintf(shaderNameDefine, "#define %s\n", shaderName);
	char vertexShaderDefine[] = "#define VERTEX\n";
	char fragmentShaderDefine[] = "#define FRAGMENT\n";

	const GLchar* vertexShaderSource[] = {
		versionString,
		shaderNameDefine,
		vertexShaderDefine,
		programSource.str
	};
	const GLint vertexShaderLengths[] = {
		(GLint)strlen(versionString),
		(GLint)strlen(shaderNameDefine),
		(GLint)strlen(vertexShaderDefine),
		(GLint)programSource.len
	};
	const GLchar* fragmentShaderSource[] = {
		versionString,
		shaderNameDefine,
		fragmentShaderDefine,
		programSource.str
	};

	const GLint fragmentShaderLengths[] = {
		(GLint)strlen(versionString),
		(GLint)strlen(shaderNameDefine),
		(GLint)strlen(fragmentShaderDefine),
		(GLint)programSource.len
	};

	GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
	glCompileShader(vshader);
	glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
		ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
	}

	GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
	glCompileShader(fshader);
	glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
		ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
	}

	GLuint programHandle = glCreateProgram();
	glAttachShader(programHandle, vshader);
	glAttachShader(programHandle, fshader);
	glLinkProgram(programHandle);
	glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
		ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
	}

	glUseProgram(0);

	glDetachShader(programHandle, vshader);
	glDetachShader(programHandle, fshader);
	glDeleteShader(vshader);
	glDeleteShader(fshader);

	return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
	String programSource = ReadTextFile(filepath);

	Program program = {};
	program.handle = CreateProgramFromSource(programSource, programName);
	program.filepath = filepath;
	program.programName = programName;
	program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);
	app->programs.push_back(program);

	return app->programs.size() - 1;
}

u8 LoadProgramAttributes(Program& program)
{
	GLsizei attributeCount;
	glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);

	for (u32 i = 0; i < attributeCount; ++i)
	{
		GLchar attributeName[128];
		GLsizei attributeNameLenght;
		GLint attributeSize;
		GLenum attributeType;
		glGetActiveAttrib(program.handle, i, ARRAY_COUNT(attributeName), &attributeNameLenght, &attributeSize, &attributeType, attributeName);

		u8 attributeLoacation = glGetAttribLocation(program.handle, attributeName);
		program.vertexInputLayout.attributes.push_back({ attributeLoacation, (u8)attributeSize });
	}

	return attributeCount;
}

Image LoadImage(const char* filename)
{
	Image img = {};
	stbi_set_flip_vertically_on_load(true);
	img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
	if (img.pixels)
	{
		img.stride = img.size.x * img.nchannels;
	}
	else
	{
		ELOG("Could not open file %s", filename);
	}
	return img;
}

void FreeImage(Image image)
{
	stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
	GLenum internalFormat = GL_RGB8;
	GLenum dataFormat = GL_RGB;
	GLenum dataType = GL_UNSIGNED_BYTE;

	switch (image.nchannels)
	{
	case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
	case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
	default: ELOG("LoadTexture2D() - Unsupported number of channels");
	}

	GLuint texHandle;
	glGenTextures(1, &texHandle);
	glBindTexture(GL_TEXTURE_2D, texHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texHandle;
}


u32 LoadTexture2D(App* app, const char* filepath)
{
	for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
		if (app->textures[texIdx].filepath == filepath)
			return texIdx;

	Image image = LoadImage(filepath);

	if (image.pixels)
	{
		Texture tex = {};
		tex.handle = CreateTexture2DFromImage(image);
		tex.filepath = filepath;
		tex.size = image.size;

		u32 texIdx = app->textures.size();
		app->textures.push_back(tex);

		FreeImage(image);
		return texIdx;
	}
	else
	{
		return UINT32_MAX;
	}
}

mat4 TransformScale(const vec3& scaleFactors)
{
	mat4 transform = glm::scale(scaleFactors);
	return transform;
}

mat4 TransformPositionScale(const vec3& pos, const vec3& scaleFactors)
{
	mat4 transform = glm::translate(pos);
	transform = glm::scale(transform, scaleFactors);
	return transform;
}

mat4 TransformPositionRotationScale(const vec3& pos, const vec3& rotation, const vec3& scaleFactors)
{
	mat4 transform = glm::translate(pos);
	transform = glm::rotate(transform, glm::radians(90.0f), rotation);
	transform = glm::scale(transform, scaleFactors);
	return transform;
}

void Init(App* app)
{
	if (GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 3))
	{
		glDebugMessageCallback(OnGlError, app);
	}

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	app->camera = Camera(vec3(3.25f, 3.5f, 3.125f));
	app->camera.yaw = -135.0f;
	app->camera.pitch = -24.0f;
	app->camera.UpdateCameraVectors();

	GLint maxUniformBufferSize;

	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBufferSize);
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBufferAlignment);

	glGenBuffers(1, &app->bufferHandle);
	glBindBuffer(GL_UNIFORM_BUFFER, app->bufferHandle);
	glBufferData(GL_UNIFORM_BUFFER, maxUniformBufferSize, NULL, GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	app->cbuffer = CreateBuffer(maxUniformBufferSize, GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW);

	//Cubemap ===============================================================================================
	GenerateCube(app);
	CreateCubemap(app);

	GenerateQuad(app);

	//Engine models
	app->directionalLightModel = LoadModel(app, "Primitives/Quad/quad.obj");
	app->pointLightModel = LoadModel(app, "Primitives/Sphere/sphere.obj");

	//Entitiy
	Entity entity;
	entity.position = vec3(0.0f, 0.0f, 0.0f);
	app->model = LoadModel(app, "Patrick/Patrick.obj");
	//app->model = LoadModel(app, "Room/Room #1.obj");
	entity.modelIndex = app->model;
	app->entities.push_back(entity);

	//Lights
	//Directional
	app->lights.push_back(CreateLight(app, LightType::LightType_Directional, vec3(2.5f, 3.0f, 0.0f), vec3(0.2f, 0.250f, 0.8f), vec3(0.9f, 0.0f, 0.0f)));
	app->lights.push_back(CreateLight(app, LightType::LightType_Directional, vec3(-2.9f, 2.75f, -2.0f), vec3(1.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, 0.9f)));

	//Point
	app->lights.push_back(CreateLight(app, LightType::LightType_Point, vec3(-1.0f, 2.75, 2.2f), vec3(1.0f), vec3(0.0f, 1.0f, 0.0f)));
	app->lights.push_back(CreateLight(app, LightType::LightType_Point, vec3(1.0f, 2.75, 2.2f), vec3(1.0f), vec3(0.5f, 0.2f, 0.5f)));
	app->lights.push_back(CreateLight(app, LightType::LightType_Point, vec3(0.1f, 1.55, -0.2f), vec3(1.0f), vec3(0.33f, 0.2f, 0.05f)));
	//app->lights.push_back(pointLight);

	// ====================================================================================================================================================

	//Rendering
	app->currentRenderMode = RenderMode::DEFERRED;
	app->currentRenderTargetMode = RenderTargetsMode::FINAL_RENDER;

	app->forwardGeometryProgramIdx = LoadProgram(app, "shaders/forward_geometry.glsl", "FORWARD_GEOMETRY");
	Program& forwardGeometryProgram = app->programs[app->forwardGeometryProgramIdx];
	LoadProgramAttributes(forwardGeometryProgram);

	//Geometry
	app->deferredGeometryProgramIdx = LoadProgram(app, "shaders/deferred_geometry.glsl", "DEFERRED_GEOMETRY");
	Program& deferredGeometryProgram = app->programs[app->deferredGeometryProgramIdx];
	LoadProgramAttributes(deferredGeometryProgram);

	app->lightsProgramIdx = LoadProgram(app, "shaders/lights.glsl", "SHOW_LIGHTS");
	Program& lightsProgram = app->programs[app->lightsProgramIdx];
	LoadProgramAttributes(lightsProgram);

	//Quad
	app->forwardQuadProgramIdx = LoadProgram(app, "shaders/forward_quad.glsl", "FORWARD_QUAD");
	Program& forwardQuadProgram = app->programs[app->forwardQuadProgramIdx];
	LoadProgramAttributes(forwardQuadProgram);
	app->programUniformTexture = glGetUniformLocation(forwardQuadProgram.handle, "uTexture");

	app->deferredQuadProgramIdx = LoadProgram(app, "shaders/deferred_quad .glsl", "DEFERRED_QUAD");
	Program& deferredQuadProgram = app->programs[app->deferredQuadProgramIdx];
	LoadProgramAttributes(deferredQuadProgram);
	app->programUniformTexture = glGetUniformLocation(deferredQuadProgram.handle, "uTexture");

	app->deferredPBRQuadProgramIdx = LoadProgram(app, "shaders/pbr_deferred_quad.glsl", "DEFERRED_PBR_QUAD");
	Program& deferredPBRQuadProgram = app->programs[app->deferredPBRQuadProgramIdx];
	LoadProgramAttributes(deferredPBRQuadProgram);

	app->depthProgramIdx = LoadProgram(app, "shaders/depth.glsl", "SHOW_DEPTH");
	Program& depthProgram = app->programs[app->depthProgramIdx];
	LoadProgramAttributes(depthProgram);

	//Cubemap
	app->cubemapProgramIdx = LoadProgram(app, "shaders/cubemap.glsl", "CUBEMAP");
	Program& cubemapProgram = app->programs[app->cubemapProgramIdx];
	LoadProgramAttributes(cubemapProgram);

	app->irradianceMapProgramIdx = LoadProgram(app, "shaders/irradiance_map.glsl", "IRRADIANCE_MAP");
	Program& irradianceMapProgram = app->programs[app->irradianceMapProgramIdx];
	LoadProgramAttributes(irradianceMapProgram);

	//Load Textures
	app->diceTexIdx = LoadTexture2D(app, "dice.png");
	app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
	app->blackTexIdx = LoadTexture2D(app, "color_black.png");
	app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
	app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");

	OnScreenResize(app);
}

void Gui(App* app)
{
	//Info window
	ImGui::Begin("Info");
	ImGui::Text("FPS: %f", 1.0f / app->deltaTime);
	ImGui::Text("OpenGL version: %s", glGetString(GL_VERSION));
	ImGui::Text("OpenGL Renderer: %s", glGetString(GL_RENDERER));
	ImGui::Text("OpenGL Vendor: %s", glGetString(GL_VENDOR));
	ImGui::Text("OpenGL GLSL version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
	if (ImGui::TreeNode("OpenGL extensions:"))
	{
		int numExtensions = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
		for (int i = 0; i < numExtensions; ++i)
		{
			const u8* str = glGetStringi(GL_EXTENSIONS, GLuint(i));
			ImGui::Text("%s", str);
		}

		ImGui::TreePop();
	}

	ImGui::End();

	ImGui::Begin("Editor");

	ImGui::Dummy(ImVec2(0.0f, 5.0f));

	ImGui::Text("Camera");

	float cameraPosition[3] = { app->camera.position.x, app->camera.position.y, app->camera.position.z };
	ImGui::DragFloat3("Camera position", cameraPosition, 0.1f, -20000000000000000.0f, 200000000000000000000.0f);
	app->camera.position = vec3(cameraPosition[0], cameraPosition[1], cameraPosition[2]);

	ImGui::DragFloat("Movement speed", &app->camera.movementSpeed, 0.05f, 0.0f, 200.0f, "%.2f", 1.0f);
	ImGui::DragFloat("Mouse Sensitivity", &app->camera.mouseSensitivity, 0.02f, 0.0f, 1.0f, "%.2f", 1.0f);


	ImGui::Dummy(ImVec2(0.0f, 10.0f));
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	const char* renderModeBuffers[] = { "FORWARD", "DEFERRED" };
	if (ImGui::BeginCombo("Render Mode", renderModeBuffers[(u32)app->currentRenderMode]))
	{
		for (size_t i = 0; i < IM_ARRAYSIZE(renderModeBuffers); ++i)
		{
			bool isSelected = (i == (u32)app->currentRenderMode);
			if (ImGui::Selectable(renderModeBuffers[i], isSelected))
			{
				app->currentRenderMode = (RenderMode)i;
			}
		}

		ImGui::EndCombo();
	}

	const char* renderTargetBuffers[] = { "ALBEDO", "NORMALS", "POSITION", "DEPTH", "METALLIC", "ROUGHNESS", "FINAL RENDER" };
	if (ImGui::BeginCombo("Render Targets", renderTargetBuffers[(u32)app->currentRenderTargetMode]))
	{
		for (size_t i = 0; i < IM_ARRAYSIZE(renderTargetBuffers); ++i)
		{
			bool isSelected = (i == (u32)app->currentRenderTargetMode);
			if (ImGui::Selectable(renderTargetBuffers[i], isSelected))
			{
				app->currentRenderTargetMode = (RenderTargetsMode)i;
			}
		}

		ImGui::EndCombo();
	}

	ImGui::Dummy(ImVec2(0.0f, 10.0f));
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	if (ImGui::TreeNode("Entities"))
	{
		for (size_t i = 0; i < app->entities.size(); ++i)
		{
			ImGui::PushID(i);
			std::string entityName = "Entity: " + std::to_string(i);

			ImGui::Text(entityName.c_str());

			Entity& entity = app->entities[i];
			float position[3] = { entity.position.x, entity.position.y, entity.position.z };
			ImGui::DragFloat3("Position", position, 0.1f, -20000000000000000.0f, 200000000000000000000.0f);
			entity.position = vec3(position[0], position[1], position[2]);

			ImGui::DragFloat("Metallic", &entity.metallic, 0.05f, 0.0f, 1.0f, "%.2f");
			ImGui::DragFloat("Roughness", &entity.roughness, 0.05f, 0.0f, 1.0f, "%.2f");

			ImGui::PopID();
		}
		ImGui::TreePop();
	}

	ImGui::Dummy(ImVec2(0.0f, 5.0f));

	if (ImGui::Button("Create Entity"))
	{
		Entity newEntity;
		newEntity.position = vec3(0.0f);
		newEntity.modelIndex = app->model;
		app->entities.push_back(newEntity);
	}

	ImGui::Dummy(ImVec2(0.0f, 7.5f));
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0.0f, 7.5f));

	if (ImGui::TreeNode("Lights"))
	{
		for (int i = 0; i < app->lights.size(); i++)
		{
			Light& light = app->lights[i];

			ImGui::PushID(i * 1000);

			std::string lightName = "Light: " + std::to_string(i);
			lightName += light.type == LightType::LightType_Directional ? " - Directional Light" : " - Point Light";
			ImGui::Text(lightName.c_str());

			ImGui::Dummy(ImVec2(0.0f, 10.0f));

			float position[3] = { light.position.x, light.position.y, light.position.z };
			ImGui::DragFloat3("Position", position, 0.1f, -20000000000000000.0f, 200000000000000000000.0f);
			light.position = vec3(position[0], position[1], position[2]);

			if (light.type == LightType::LightType_Directional)
			{
				float direction[3] = { light.direction.x, light.direction.y, light.direction.z };
				ImGui::DragFloat3("Direction", direction, 0.1f, -20000000000000000.0f, 200000000000000000000.0f);
				light.direction = vec3(direction[0], direction[1], direction[2]);
			}

			float color[3] = { light.color.r, light.color.g, light.color.b };
			ImGui::ColorPicker3("Color", color);
			light.color = vec3(color[0], color[1], color[2]);
			ImGui::PopID();

			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.0f, 15.0f));
		}
		ImGui::TreePop();
	}

	ImGui::Dummy(ImVec2(0.0f, 7.5f));

	if (ImGui::Button("Create Directional Light"))
	{
		app->lights.push_back(CreateLight(app, LightType::LightType_Directional, vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f)));
	}

	if (ImGui::Button("Create Point Light"))
	{
		app->lights.push_back(CreateLight(app, LightType::LightType_Point, vec3(0.0f, 1.5f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f)));
	}

	ImGui::End();
}

void Update(App* app)
{
	// You can handle app->input keyboard/mouse here
	HandleInput(app);

	for (u64 i = 0; i < app->programs.size(); i++)
	{
		Program& program = app->programs[i];
		u64 currentTimestamp = GetFileLastWriteTimestamp(program.filepath.c_str());
		if (currentTimestamp > program.lastWriteTimestamp)
		{
			glDeleteProgram(program.handle);
			String programSource = ReadTextFile(program.filepath.c_str());
			const char* programName = program.programName.c_str();
			program.handle = CreateProgramFromSource(programSource, programName);
			program.lastWriteTimestamp = currentTimestamp;
		}
	}

	float aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
	float znear = 0.1f;
	float zfar = 1000.0f;

	mat4 projection = glm::perspective(glm::radians(app->camera.zoom), aspectRatio, znear, zfar);
	mat4 view = app->camera.GetViewMatrix();

	MapBuffer(app->cbuffer, GL_WRITE_ONLY);

	//Global params
	app->globalParamsOffset = app->cbuffer.head;

	PushUInt(app->cbuffer, (u32)app->currentRenderTargetMode);
	PushVec3(app->cbuffer, app->camera.position);
	PushUInt(app->cbuffer, app->lights.size());

	for (u32 i = 0; i < app->lights.size(); ++i)
	{
		AlignHead(app->cbuffer, sizeof(vec4));

		Light& light = app->lights[i];
		PushUInt(app->cbuffer, (u32)light.type);
		PushVec3(app->cbuffer, light.color);
		PushVec3(app->cbuffer, light.direction);
		PushVec3(app->cbuffer, light.position);
	}

	app->globalParamsSize = app->cbuffer.head - app->globalParamsOffset;

	//Normal entities
	for (size_t i = 0; i < app->entities.size(); ++i)
	{
		AlignHead(app->cbuffer, app->uniformBufferAlignment);

		Entity& entity = app->entities[i];
		mat4 world = entity.worldMatrix;
		world = TransformPositionScale(entity.position, vec3(0.45f));
		mat4 worldViewProjection = projection * view * world;

		entity.localParamsOffset = app->cbuffer.head;
		PushMat4(app->cbuffer, world);
		PushMat4(app->cbuffer, worldViewProjection);
		entity.localParamsSize = app->cbuffer.head - entity.localParamsOffset;
	}

	////Light entities
	//for (size_t i = 0; i < app->lights.size(); i++)
	//{
	//	AlignHead(app->cbuffer, app->uniformBufferAlignment);

	//	Light& light = app->lights[i];
	//	Entity& entity = light.entity;
	//	entity.position = light.position;

	//	mat4 world = entity.worldMatrix;
	//	world = TransformPositionRotationScale(light.position, light.direction, light.type == LightType::LightType_Directional ? vec3(0.45f) : vec3(0.25f));
	//	mat4 worldViewProjection = projection * view * world;

	//	entity.localParamsOffset = app->cbuffer.head;
	//	PushMat4(app->cbuffer, world);
	//	PushMat4(app->cbuffer, worldViewProjection);
	//	entity.localParamsSize = app->cbuffer.head - entity.localParamsOffset;
	//}

	UnmapBuffer(app->cbuffer);
}


void Render(App* app)
{
	//Render on this framebuffer render targets
	glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);

	//Select on which render targets to draw
	GLuint drawBuffers[] = { app->albedoAttachmentHandle, app->normalsAttachmentHandle,
		app->positionAttachmentHandle, app->metallicAttachmentHandle, app->roughnessAttachmentHandle,
		app->finalRenderAttachmentHandle };

	glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);

	//Clear color and depth
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	//FORWARD SHADING =======================================================================================
	if (app->currentRenderMode == RenderMode::FORWARD)
	{
		ForwardRender(app);
	}

	//DEFERRED SHADING ======================================================================================
	else
	{
		DeferredRender(app);
	}
}

void ForwardRender(App* app)
{
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Forward shaded model");

	Program& modelProgram = app->programs[app->forwardGeometryProgramIdx];
	glUseProgram(modelProgram.handle);

	for (size_t i = 0; i < app->entities.size(); ++i)
	{
		Entity& entity = app->entities[i];
		RenderModel(app, entity, modelProgram);
	}

	glPopDebugGroup();

	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Forward lights");

	Program& lightsProgram = app->programs[app->lightsProgramIdx];
	glUseProgram(lightsProgram.handle);

	if (app->currentRenderTargetMode == RenderTargetsMode::FINAL_RENDER)
	{
		for (size_t i = 0; i < app->lights.size(); i++)
		{
			Light& light = app->lights[i];
			RenderLight(app, light, lightsProgram);
		}
	}
	glPopDebugGroup();


	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Cubemap");

	float aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
	float znear = 0.1f;
	float zfar = 1000.0f;
	mat4 projection = glm::perspective(glm::radians(app->camera.zoom), aspectRatio, znear, zfar);
	mat4 view = glm::mat4(glm::mat3(app->camera.GetViewMatrix()));

	DrawCube(app, app->cubemapProgramIdx, app->cubemapAttachmentHandle, true, view, projection);

	glPopDebugGroup();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Forward Textured quad");

	DrawQuad(app);

	glPopDebugGroup();
}

void DeferredRender(App* app)
{
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Deferred Shaded model");

	Program& modelProgram = app->programs[app->deferredGeometryProgramIdx];
	glUseProgram(modelProgram.handle);

	for (size_t i = 0; i < app->entities.size(); ++i)
	{
		Entity& entity = app->entities[i];
		RenderModel(app, entity, modelProgram);
	}

	glPopDebugGroup();

	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Deferred Lights");

	Program& lightsProgram = app->programs[app->lightsProgramIdx];
	glUseProgram(lightsProgram.handle);

	for (size_t i = 0; i < app->lights.size(); i++)
	{
		Light& light = app->lights[i];
		//RenderLight(app, light, lightsProgram);
	}

	glPopDebugGroup();

	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Cubemap");

	float aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
	float znear = 0.1f;
	float zfar = 1000.0f;
	mat4 projection = glm::perspective(glm::radians(app->camera.zoom), aspectRatio, znear, zfar);
	mat4 view = glm::mat4(glm::mat3(app->camera.GetViewMatrix()));

	CreateIrradianceMap(app);
	DrawCube(app, app->cubemapProgramIdx, app->irradianceMapAttachmentHandle, true, view, projection);

	glPopDebugGroup();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Deferred Textured quad");

	DrawQuad(app);

	glPopDebugGroup();
}

void RenderModel(App* app, Entity entity, Program program)
{
	Model& model = app->models[entity.modelIndex];
	Mesh& mesh = app->meshes[model.meshIdx];

	glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cbuffer.handle, app->globalParamsOffset, app->globalParamsSize);
	glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->cbuffer.handle, entity.localParamsOffset, entity.localParamsSize);

	for (u32 j = 0; j < mesh.submeshes.size(); ++j)
	{
		GLuint vao = FindVAO(mesh, j, program);
		glBindVertexArray(vao);

		u32 submeshMaterialIdx = model.materialIdx[j];
		Material& submeshMaterial = app->materials[submeshMaterialIdx];

		if (submeshMaterial.albedoTextureIdx < app->textures.size())
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
			GLuint textureLocation = glGetUniformLocation(program.handle, "uTexture");
			glUniform1i(textureLocation, 0);
		}

		GLuint metallicLocation = glGetUniformLocation(program.handle, "uMetallic");
		glUniform1f(metallicLocation, entity.metallic);

		GLuint roughnessLocation = glGetUniformLocation(program.handle, "uRoughness");
		glUniform1f(roughnessLocation, entity.roughness);

		Submesh& submesh = mesh.submeshes[j];
		glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void RenderLight(App* app, Light light, Program program)
{
	Model& model = app->models[light.entity.modelIndex];
	Mesh& mesh = app->meshes[model.meshIdx];

	glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cbuffer.handle, app->globalParamsOffset, app->globalParamsSize);
	glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->cbuffer.handle, light.entity.localParamsOffset, light.entity.localParamsSize);

	for (u32 j = 0; j < mesh.submeshes.size(); ++j)
	{
		GLuint vao = FindVAO(mesh, j, program);
		glBindVertexArray(vao);

		GLuint lightColorLocation = glGetUniformLocation(program.handle, "uLightColor");
		glUniform3f(lightColorLocation, light.color.r, light.color.g, light.color.b);

		Submesh& submesh = mesh.submeshes[j];
		glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
	}

	glUnmapBuffer(GL_UNIFORM_BUFFER);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void OnGlError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
		return;
	}

	ELOG("OpenGL debug message: %s", message);

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:				ELOG(" - source: GL_DEBUG_SOURCE_API"); break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:		ELOG(" - source: GL_DEBUG_SOURCE_WINDOW_SYSTEM"); break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:	ELOG(" - source: GL_DEBUG_SOURCE_SHADER_COMPILER"); break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:		ELOG(" - source: GL_DEBUG_SOURCE_THIRD_PARTY"); break;
	case GL_DEBUG_SOURCE_APPLICATION:		ELOG(" - source: GL_DEBUG_SOURCE_APPLICATION"); break;
	case GL_DEBUG_SOURCE_OTHER:				ELOG(" - source: GL_DEBUG_SOURCE_OTHER");  break;
	}

	switch (source)
	{
	case GL_DEBUG_TYPE_ERROR:				ELOG(" - source: GL_DEBUG_TYPE_ERROR"); break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:	ELOG(" - source: GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR"); break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:	ELOG(" - source: GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR"); break;
	case GL_DEBUG_TYPE_PORTABILITY:			ELOG(" - source: GL_DEBUG_TYPE_PORTABILITY"); break;
	case GL_DEBUG_TYPE_PERFORMANCE:			ELOG(" - source: GL_DEBUG_TYPE_PERFORMANCE"); break;
	case GL_DEBUG_TYPE_MARKER:				ELOG(" - source: GL_DEBUG_TYPE_MARKER"); break;
	case GL_DEBUG_TYPE_PUSH_GROUP:			ELOG(" - source: GL_DEBUG_TYPE_PUSH_GROUP"); break;
	case GL_DEBUG_TYPE_POP_GROUP:			ELOG(" - source: GL_DEBUG_TYPE_POP_GROUP"); break;
	case GL_DEBUG_TYPE_OTHER:				ELOG(" - source: GL_DEBUG_TYPE_OTHER"); break;
	}

	switch (source)
	{
	case GL_DEBUG_SEVERITY_HIGH:			ELOG(" - source: GL_DEBUG_SEVERITY_HIGH"); break;
	case GL_DEBUG_SEVERITY_MEDIUM:			ELOG(" - source: GL_DEBUG_SEVERITY_MEDIUM"); break;
	case GL_DEBUG_SEVERITY_LOW:				ELOG(" - source: GL_DEBUG_SEVERITY_LOW"); break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:	ELOG(" - source: GL_DEBUG_SEVERITY_NOTIFICATION"); break;
	}
}

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
	Submesh& submesh = mesh.submeshes[submeshIndex];

	//Try finding a vao for this submesh/program
	for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
	{
		if (submesh.vaos[i].programHandle == program.handle)
		{
			return submesh.vaos[i].handle;
		}
	}

	GLuint vaoHandle = 0;

	//Create a new vao for this submesh/program
	glGenVertexArrays(1, &vaoHandle);
	glBindVertexArray(vaoHandle);

	glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

	for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i)
	{
		bool attributeWasLinked = false;

		for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j)
		{
			if (program.vertexInputLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location)
			{
				const u32 index = submesh.vertexBufferLayout.attributes[j].location;
				const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
				const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset;  //attribute offset + vertex offset
				const u32 stride = submesh.vertexBufferLayout.stride;
				glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
				glEnableVertexAttribArray(index);

				attributeWasLinked = true;
				break;
			}
		}

		//assert(attributeWasLinked); //The submesh should provide an attribute for each vertex input
	}

	glBindVertexArray(0);

	//Store it in the list of vas for this submesh
	Vao vao = { vaoHandle, program.handle };
	submesh.vaos.push_back(vao);

	return vaoHandle;
}

void OnScreenResize(App* app)
{
	GenerateColorTexture(app->albedoAttachmentHandle, app->displaySize, GL_RGBA8);
	GenerateColorTexture(app->normalsAttachmentHandle, app->displaySize, GL_RGBA16F);
	GenerateColorTexture(app->positionAttachmentHandle, app->displaySize, GL_RGBA16F);
	GenerateColorTexture(app->metallicAttachmentHandle, app->displaySize, GL_RGBA16F);
	GenerateColorTexture(app->roughnessAttachmentHandle, app->displaySize, GL_RGBA16F);
	GenerateColorTexture(app->finalRenderAttachmentHandle, app->displaySize, GL_RGBA16F);

	glGenTextures(1, &app->depthAttachmentHandle);
	glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);


	glGenFramebuffers(1, &app->framebufferHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->albedoAttachmentHandle, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->normalsAttachmentHandle, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, app->positionAttachmentHandle, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, app->metallicAttachmentHandle, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, app->roughnessAttachmentHandle, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, app->finalRenderAttachmentHandle, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->depthAttachmentHandle, 0);

	GLenum buffers[] = { GL_COLOR_ATTACHMENT0,  GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
		GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5 };
	glDrawBuffers(6, buffers);

	GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		switch (framebufferStatus)
		{
		case GL_FRAMEBUFFER_UNDEFINED:						ELOG("GL_FRAMEBUFFER_UNDEFINED"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:			ELOG("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:	ELOG("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:			ELOG("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:			ELOG("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
		case GL_FRAMEBUFFER_UNSUPPORTED:					ELOG("GL_FRAMEBUFFER_UNSUPPORTED"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:			ELOG("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:		ELOG("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;
		default:											ELOG("Unknown framebuffer status error") break;
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	CreateIrradianceMap(app);
}

void HandleInput(App* app)
{
	if (app->input.keys[K_W] == BUTTON_PRESSED) {
		app->camera.ProcessKeyboard(Camera_Movement::CAMERA_FORWARD, app->deltaTime);
	}
	if (app->input.keys[K_A] == BUTTON_PRESSED) {
		app->camera.ProcessKeyboard(Camera_Movement::CAMERA_LEFT, app->deltaTime);
	}
	if (app->input.keys[K_S] == BUTTON_PRESSED) {
		app->camera.ProcessKeyboard(Camera_Movement::CAMERA_BACKWARD, app->deltaTime);
	}
	if (app->input.keys[K_D] == BUTTON_PRESSED) {
		app->camera.ProcessKeyboard(Camera_Movement::CAMERA_RIGHT, app->deltaTime);
	}

	if (app->input.mouseButtons[LEFT] == BUTTON_PRESSED || app->input.mouseButtons[RIGHT] == BUTTON_PRESSED)
	{
		app->camera.ProcessMouseMovement(app->input.mouseDelta.x, -app->input.mouseDelta.y);
	}

	//app->camera.ProcessMouseScroll(app->input.mouseDelta);
}

void ProcessAssimpMesh(const aiScene* scene, aiMesh* mesh, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices)
{
	std::vector<float> vertices;
	std::vector<u32> indices;

	bool hasTexCoords = false;
	bool hasTangentSpace = false;

	// process vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		vertices.push_back(mesh->mVertices[i].x);
		vertices.push_back(mesh->mVertices[i].y);
		vertices.push_back(mesh->mVertices[i].z);
		vertices.push_back(mesh->mNormals[i].x);
		vertices.push_back(mesh->mNormals[i].y);
		vertices.push_back(mesh->mNormals[i].z);

		if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
		{
			hasTexCoords = true;
			vertices.push_back(mesh->mTextureCoords[0][i].x);
			vertices.push_back(mesh->mTextureCoords[0][i].y);
		}

		if (mesh->mTangents != nullptr && mesh->mBitangents)
		{
			hasTangentSpace = true;
			vertices.push_back(mesh->mTangents[i].x);
			vertices.push_back(mesh->mTangents[i].y);
			vertices.push_back(mesh->mTangents[i].z);

			// For some reason ASSIMP gives me the bitangents flipped.
			// Maybe it's my fault, but when I generate my own geometry
			// in other files (see the generation of standard assets)
			// and all the bitangents have the orientation I expect,
			// everything works ok.
			// I think that (even if the documentation says the opposite)
			// it returns a left-handed tangent space matrix.
			// SOLUTION: I invert the components of the bitangent here.
			vertices.push_back(-mesh->mBitangents[i].x);
			vertices.push_back(-mesh->mBitangents[i].y);
			vertices.push_back(-mesh->mBitangents[i].z);
		}
	}

	// process indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	// store the proper (previously proceessed) material for this mesh
	submeshMaterialIndices.push_back(baseMeshMaterialIndex + mesh->mMaterialIndex);

	// create the vertex format
	VertexBufferLayout vertexBufferLayout = {};
	vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 0, 3, 0 });
	vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 1, 3, 3 * sizeof(float) });
	vertexBufferLayout.stride = 6 * sizeof(float);
	if (hasTexCoords)
	{
		vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 2, 2, vertexBufferLayout.stride });
		vertexBufferLayout.stride += 2 * sizeof(float);
	}
	if (hasTangentSpace)
	{
		vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 3, 3, vertexBufferLayout.stride });
		vertexBufferLayout.stride += 3 * sizeof(float);

		vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 4, 3, vertexBufferLayout.stride });
		vertexBufferLayout.stride += 3 * sizeof(float);
	}

	// add the submesh into the mesh
	Submesh submesh = {};
	submesh.vertexBufferLayout = vertexBufferLayout;
	submesh.vertices.swap(vertices);
	submesh.indices.swap(indices);
	myMesh->submeshes.push_back(submesh);
}

void ProcessAssimpMaterial(App* app, aiMaterial* material, Material& myMaterial, String directory)
{
	aiString name;
	aiColor3D diffuseColor;
	aiColor3D emissiveColor;
	aiColor3D specularColor;
	ai_real shininess;
	material->Get(AI_MATKEY_NAME, name);
	material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
	material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor);
	material->Get(AI_MATKEY_COLOR_SPECULAR, specularColor);
	material->Get(AI_MATKEY_SHININESS, shininess);

	myMaterial.name = name.C_Str();
	myMaterial.albedo = vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b);
	myMaterial.emissive = vec3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
	myMaterial.smoothness = shininess / 256.0f;

	aiString aiFilename;
	if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
	{
		material->GetTexture(aiTextureType_DIFFUSE, 0, &aiFilename);
		String filename = MakeString(aiFilename.C_Str());
		String filepath = MakePath(directory, filename);
		myMaterial.albedoTextureIdx = LoadTexture2D(app, filepath.str);
	}
	if (material->GetTextureCount(aiTextureType_EMISSIVE) > 0)
	{
		material->GetTexture(aiTextureType_EMISSIVE, 0, &aiFilename);
		String filename = MakeString(aiFilename.C_Str());
		String filepath = MakePath(directory, filename);
		myMaterial.emissiveTextureIdx = LoadTexture2D(app, filepath.str);
	}
	if (material->GetTextureCount(aiTextureType_SPECULAR) > 0)
	{
		material->GetTexture(aiTextureType_SPECULAR, 0, &aiFilename);
		String filename = MakeString(aiFilename.C_Str());
		String filepath = MakePath(directory, filename);
		myMaterial.specularTextureIdx = LoadTexture2D(app, filepath.str);
	}
	if (material->GetTextureCount(aiTextureType_NORMALS) > 0)
	{
		material->GetTexture(aiTextureType_NORMALS, 0, &aiFilename);
		String filename = MakeString(aiFilename.C_Str());
		String filepath = MakePath(directory, filename);
		myMaterial.normalsTextureIdx = LoadTexture2D(app, filepath.str);
	}
	if (material->GetTextureCount(aiTextureType_HEIGHT) > 0)
	{
		material->GetTexture(aiTextureType_HEIGHT, 0, &aiFilename);
		String filename = MakeString(aiFilename.C_Str());
		String filepath = MakePath(directory, filename);
		myMaterial.bumpTextureIdx = LoadTexture2D(app, filepath.str);
	}

	//myMaterial.createNormalFromBump();
}

void ProcessAssimpNode(const aiScene* scene, aiNode* node, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices)
{
	// process all the node's meshes (if any)
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessAssimpMesh(scene, mesh, myMesh, baseMeshMaterialIndex, submeshMaterialIndices);
	}

	// then do the same for each of its children
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		ProcessAssimpNode(scene, node->mChildren[i], myMesh, baseMeshMaterialIndex, submeshMaterialIndices);
	}
}

u32 LoadModel(App* app, const char* filename)
{
	const aiScene* scene = aiImportFile(filename,
		aiProcess_Triangulate |
		aiProcess_GenSmoothNormals |
		aiProcess_CalcTangentSpace |
		aiProcess_JoinIdenticalVertices |
		aiProcess_PreTransformVertices |
		aiProcess_ImproveCacheLocality |
		aiProcess_OptimizeMeshes |
		aiProcess_SortByPType);

	if (!scene)
	{
		ELOG("Error loading mesh %s: %s", filename, aiGetErrorString());
		return UINT32_MAX;
	}

	app->meshes.push_back(Mesh{});
	Mesh& mesh = app->meshes.back();
	u32 meshIdx = (u32)app->meshes.size() - 1u;

	app->models.push_back(Model{});
	Model& model = app->models.back();
	model.meshIdx = meshIdx;
	u32 modelIdx = (u32)app->models.size() - 1u;

	String directory = GetDirectoryPart(MakeString(filename));

	// Create a list of materials
	u32 baseMeshMaterialIndex = (u32)app->materials.size();
	for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
	{
		app->materials.push_back(Material{});
		Material& material = app->materials.back();
		ProcessAssimpMaterial(app, scene->mMaterials[i], material, directory);
	}

	ProcessAssimpNode(scene, scene->mRootNode, &mesh, baseMeshMaterialIndex, model.materialIdx);

	aiReleaseImport(scene);

	u32 vertexBufferSize = 0;
	u32 indexBufferSize = 0;

	for (u32 i = 0; i < mesh.submeshes.size(); ++i)
	{
		vertexBufferSize += mesh.submeshes[i].vertices.size() * sizeof(float);
		indexBufferSize += mesh.submeshes[i].indices.size() * sizeof(u32);
	}

	glGenBuffers(1, &mesh.vertexBufferHandle);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, NULL, GL_STATIC_DRAW);

	glGenBuffers(1, &mesh.indexBufferHandle);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, NULL, GL_STATIC_DRAW);

	u32 indicesOffset = 0;
	u32 verticesOffset = 0;

	for (u32 i = 0; i < mesh.submeshes.size(); ++i)
	{
		const void* verticesData = mesh.submeshes[i].vertices.data();
		const u32   verticesSize = mesh.submeshes[i].vertices.size() * sizeof(float);
		glBufferSubData(GL_ARRAY_BUFFER, verticesOffset, verticesSize, verticesData);
		mesh.submeshes[i].vertexOffset = verticesOffset;
		verticesOffset += verticesSize;

		const void* indicesData = mesh.submeshes[i].indices.data();
		const u32   indicesSize = mesh.submeshes[i].indices.size() * sizeof(u32);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, indicesOffset, indicesSize, indicesData);
		mesh.submeshes[i].indexOffset = indicesOffset;
		indicesOffset += indicesSize;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return modelIdx;
}

bool IsPowerOf2(u32 value)
{
	return value && !(value & (value - 1));
}

u32 Align(u32 value, u32 alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

Buffer CreateBuffer(u32 size, GLenum type, GLenum usage)
{
	Buffer buffer = {};
	buffer.size = size;
	buffer.type = type;

	glGenBuffers(1, &buffer.handle);
	glBindBuffer(type, buffer.handle);
	glBufferData(type, buffer.size, NULL, usage);
	glBindBuffer(type, 0);

	return buffer;
}

void BindBuffer(const Buffer& buffer)
{
	glBindBuffer(buffer.type, buffer.handle);
}

void MapBuffer(Buffer& buffer, GLenum access)
{
	glBindBuffer(buffer.type, buffer.handle);
	buffer.data = (u8*)glMapBuffer(buffer.type, access);
	buffer.head = 0;
}

void UnmapBuffer(Buffer& buffer)
{
	glUnmapBuffer(buffer.type);
	glBindBuffer(buffer.type, 0);
}

void AlignHead(Buffer& buffer, u32 alignment)
{
	ASSERT(IsPowerOf2(alignment), "The alignment must be a power of 2");
	buffer.head = Align(buffer.head, alignment);
}

void PushAlignedData(Buffer& buffer, const void* data, u32 size, u32 alignment)
{
	ASSERT(buffer.data != NULL, "The buffer must be mapped first");
	AlignHead(buffer, alignment);
	memcpy((u8*)buffer.data + buffer.head, data, size);
	buffer.head += size;
}

void GenerateColorTexture(GLuint& colorAttachmentHandle, vec2 displaySize, GLint internalFormat)
{
	glGenTextures(1, &colorAttachmentHandle);
	glBindTexture(GL_TEXTURE_2D, colorAttachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, displaySize.x, displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GenerateQuad(App* app)
{
	//Geometry
	glGenBuffers(1, &app->quad.embeddedVertices);
	glBindBuffer(GL_ARRAY_BUFFER, app->quad.embeddedVertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(app->quad.vertices), app->quad.vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &app->quad.embeddedElements);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->quad.embeddedElements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(app->quad.indices), app->quad.indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	//Attribute state
	glGenVertexArrays(1, &app->quad.vao);
	glBindVertexArray(app->quad.vao);
	glBindBuffer(GL_ARRAY_BUFFER, app->quad.embeddedVertices);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->quad.embeddedElements);
	glBindVertexArray(0);
}

void DrawQuad(App* app)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, app->displaySize.x, app->displaySize.y);

	//Program& quadProgram = app->currentRenderMode == RenderMode::FORWARD ? app->programs[app->forwardQuadProgramIdx] : app->programs[app->deferredQuadProgramIdx];
	Program& quadProgram = app->currentRenderMode == RenderMode::FORWARD ? app->programs[app->forwardQuadProgramIdx] : app->programs[app->deferredPBRQuadProgramIdx];

	if (app->currentRenderMode == RenderMode::FORWARD && app->currentRenderTargetMode == RenderTargetsMode::DEPTH) {
		quadProgram = app->programs[app->depthProgramIdx];
	}

	glUseProgram(quadProgram.handle);
	glBindVertexArray(app->quad.vao);

	//FORWARD
	if (app->currentRenderMode == RenderMode::FORWARD)
	{
		glUniform1i(app->programUniformTexture, 0);
		glActiveTexture(GL_TEXTURE0);

		switch (app->currentRenderTargetMode)
		{
		case RenderTargetsMode::ALBEDO:		  glBindTexture(GL_TEXTURE_2D, app->albedoAttachmentHandle); break;
		case RenderTargetsMode::NORMALS:	  glBindTexture(GL_TEXTURE_2D, app->normalsAttachmentHandle); break;
		case RenderTargetsMode::POSITION:	  glBindTexture(GL_TEXTURE_2D, app->positionAttachmentHandle); break;
		case RenderTargetsMode::DEPTH:        glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle); break;
		case RenderTargetsMode::METALLIC:	  glBindTexture(GL_TEXTURE_2D, app->metallicAttachmentHandle); break;
		case RenderTargetsMode::ROUGHNESS:	  glBindTexture(GL_TEXTURE_2D, app->roughnessAttachmentHandle); break;
		case RenderTargetsMode::FINAL_RENDER: glBindTexture(GL_TEXTURE_2D, app->finalRenderAttachmentHandle); break;
		default:							  glBindTexture(GL_TEXTURE_2D, app->albedoAttachmentHandle); break;
		}
	}
	//DEFERRED
	else
	{
		Program& deferredQuadProgram = app->programs[app->deferredPBRQuadProgramIdx];

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, app->albedoAttachmentHandle);
		GLuint colorTextureLocation = glGetUniformLocation(deferredQuadProgram.handle, "uColor");
		glUniform1i(colorTextureLocation, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, app->normalsAttachmentHandle);
		GLuint normalsTextureLocation = glGetUniformLocation(deferredQuadProgram.handle, "uNormals");
		glUniform1i(normalsTextureLocation, 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, app->positionAttachmentHandle);
		GLuint positionTextureLocation = glGetUniformLocation(deferredQuadProgram.handle, "uPosition");
		glUniform1i(positionTextureLocation, 2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, app->metallicAttachmentHandle);
		GLuint metallicTextureLocation = glGetUniformLocation(deferredQuadProgram.handle, "uMetallic");
		glUniform1i(metallicTextureLocation, 3);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, app->roughnessAttachmentHandle);
		GLuint roughnessTextureLocation = glGetUniformLocation(deferredQuadProgram.handle, "uRoughness");
		glUniform1i(roughnessTextureLocation, 4);

		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);
		GLuint depthTextureLocation = glGetUniformLocation(deferredQuadProgram.handle, "uDepth");
		glUniform1i(depthTextureLocation, 5);

		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_CUBE_MAP, app->cubemapAttachmentHandle);
		GLuint cubemapLocation = glGetUniformLocation(deferredQuadProgram.handle, "cubemap");
		glUniform1i(cubemapLocation, 6);
	}

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

	glBindVertexArray(0);
	glUseProgram(0);
}

void CreateCubemap(App* app)
{
	glGenTextures(1, &app->cubemapAttachmentHandle);
	glBindTexture(GL_TEXTURE_CUBE_MAP, app->cubemapAttachmentHandle);

	std::string path = "CubeMap/";
	std::string directions[6] = { "right", "left", "top", "bottom", "front", "back" };

	int width, height, nrChannels;
	for (u32 i = 0; i < 6; i++)
	{
		std::string texturePath = std::string(path + directions[i] + ".jpg");

		unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);

		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);

			stbi_image_free(data);
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void GenerateCube(App* app)
{
	//Attribute state
	glGenVertexArrays(1, &app->cube.vao);
	glGenBuffers(1, &app->cube.vbo);
	glBindVertexArray(app->cube.vao);
	glBindBuffer(GL_ARRAY_BUFFER, app->cube.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(app->cube.vertices), &app->cube.vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

void DrawCube(App* app, u32 programIdx, u32 programHandle, bool useViewAndProjection, mat4 view = mat4(), mat4 projection = mat4())
{
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);

	if (useViewAndProjection)
	{
		Program& cubemapProgram = app->programs[programIdx];
		glUseProgram(cubemapProgram.handle);

		GLuint projectionLocation = glGetUniformLocation(cubemapProgram.handle, "projection");
		glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, &projection[0][0]);

		GLuint viewLocation = glGetUniformLocation(cubemapProgram.handle, "view");
		glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, programHandle);
	}

	glBindVertexArray(app->cube.vao);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	if (useViewAndProjection)
	{
		glUseProgram(0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}
}

void CreateIrradianceMap(App* app)
{
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "IrradianceMap");

	glGenFramebuffers(1, &app->irradianceFramebufferHandle);
	
	glGenRenderbuffers(1, &app->renderBufferHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, app->irradianceFramebufferHandle);
	glBindRenderbuffer(GL_RENDERBUFFER, app->renderBufferHandle);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, app->renderBufferHandle);

	glGenTextures(1, &app->irradianceMapAttachmentHandle);
	glBindTexture(GL_TEXTURE_CUBE_MAP, app->irradianceMapAttachmentHandle);

	for (u32 i = 0; i < 6; i++)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, app->irradianceFramebufferHandle);
	glBindRenderbuffer(GL_RENDERBUFFER, app->renderBufferHandle);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

	Program& irradianceMapProgram = app->programs[app->irradianceMapProgramIdx];
	glUseProgram(irradianceMapProgram.handle);

	glm::mat4 captureViews[] =
	{
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	GLuint projectionLocation = glGetUniformLocation(irradianceMapProgram.handle, "projection");
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, &captureProjection[0][0]);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, app->cubemapAttachmentHandle);
	GLuint textureLocation = glGetUniformLocation(irradianceMapProgram.handle, "environmentMap");
	glUniform1d(textureLocation, 0);

	glViewport(0, 0, 32, 32);
	glBindFramebuffer(GL_FRAMEBUFFER, app->irradianceFramebufferHandle);

	for (size_t i = 0; i < 6; i++)
	{
		GLuint viewLocation = glGetUniformLocation(irradianceMapProgram.handle, "view");
		glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &captureViews[i][0][0]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, app->irradianceMapAttachmentHandle, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		DrawCube(app, app->irradianceMapProgramIdx, app->cubemapAttachmentHandle, false);
	}

	glUseProgram(0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glViewport(0, 0, app->displaySize.x, app->displaySize.y);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glPopDebugGroup();
}

Light CreateLight(App* app, LightType lightType, vec3 position, vec3 direction, vec3 color)
{
	Light light;
	light.type = lightType;
	light.position = position;
	light.color = color;
	light.direction = direction;

	Entity entity;
	entity.position = position;

	if (lightType == LightType::LightType_Directional) { entity.modelIndex = app->directionalLightModel; }
	else if (lightType == LightType::LightType_Point) { entity.modelIndex = app->pointLightModel; }

	light.entity = entity;

	return light;
}

Camera::Camera()
{
	position = vec3(0.0f, 0.0f, 0.0f);
	front = vec3(0.0f, -1.0f, 0.0f);
	right = vec3(1.0f, 0.0f, 0.0f);
	up = worldUp = vec3(0.0f, 1.0f, 0.0f);
	movementSpeed = SPEED;
	mouseSensitivity = SENSITIVITY;
	zoom = ZOOM;
}

Camera::Camera(glm::vec3 _position, glm::vec3 _up, float _yaw, float _pitch) : front(glm::vec3(0.0f, 0.0f, -1.0f)), movementSpeed(SPEED), mouseSensitivity(SENSITIVITY), zoom(ZOOM)
{
	position = _position;
	worldUp = _up;
	yaw = _yaw;
	pitch = _pitch;
	UpdateCameraVectors();
}

mat4 Camera::GetViewMatrix()
{
	return glm::lookAt(position, position + front, up);
}

void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
{
	float velocity = movementSpeed * deltaTime;
	if (direction == CAMERA_FORWARD)
		position += front * velocity;
	if (direction == CAMERA_BACKWARD)
		position -= front * velocity;
	if (direction == CAMERA_LEFT)
		position -= right * velocity;
	if (direction == CAMERA_RIGHT)
		position += right * velocity;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch)
{
	xoffset *= mouseSensitivity;
	yoffset *= mouseSensitivity;

	yaw += xoffset;
	pitch += yoffset;

	// make sure that when pitch is out of bounds, screen doesn't get flipped
	if (constrainPitch)
	{
		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;
	}

	// update Front, Right and Up Vectors using the updated Euler angles
	UpdateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset)
{
	zoom -= (float)yoffset;
	if (zoom < 1.0f) { zoom = 1.0f; }
	if (zoom > 45.0f) { zoom = 45.0f; }
}

void Camera::UpdateCameraVectors()
{
	// calculate the new Front vector
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	front = glm::normalize(front);
	// also re-calculate the Right and Up vector
	right = glm::normalize(glm::cross(front, worldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
	up = glm::normalize(glm::cross(right, front));
}
