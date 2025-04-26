//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include "buffer_management.h"
#include <glad/glad.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

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

struct OpenGL_Info
{
	char* version;
	char* renderer;
	char* vendor;
	char* shading_language_version;
	std::vector<std::string> extensions;
};

enum Mode
{
	Mode_TexturedQuad,
	Mode_Count,
	Mode_Mesh,
	Mode_Framebuffer,
	Mode_Albedo,
	Mode_Normal,
	Mode_Position,
	Mode_Depth,
};

struct VertexV3V2
{
	glm::vec3 pos;
	glm::vec2 uv;
};

const VertexV3V2 vertices[] =
{
	{ glm::vec3(-1.0, -1.0, 0.0),  glm::vec2(0.0, 0.0) },
	{ glm::vec3(1.0, -1.0, 0.0),   glm::vec2(1.0, 0.0) },
	{ glm::vec3(1.0,  1.0, 0.0),   glm::vec2(1.0, 1.0) },
	{ glm::vec3(-1.0,  1.0, 0.0),  glm::vec2(0.0, 1.0) },
};

const u16 indices[] = { 0,1,2,0,2,3 };

struct VertexBufferAttribute
{
	u8 location;
	u8 componentCount;
	u8 offset;
};

struct VertexBufferLayout
{
	std::vector<VertexBufferAttribute> vbAttributes;
	u8 stride; // size of the mesh attributes. OpenGL needs this to read the buffer
};

struct VertexShaderAttribute
{
	u8 location;
	u8 componentCount;
};

struct VertexShaderLayout
{
	std::vector<VertexShaderAttribute> vsAttributes;
};

struct Vao
{
	GLuint handle;
	GLuint programHandle;
};

struct Submesh
{
	// where we store attributes
	VertexBufferLayout vbLayout;

	// will be merged in the VBO (Vertex Buffer Object) and IBO (Index Buffer Object) in the Mesh
	std::vector<float> vertices;
	std::vector<u32> indices;

	// to find the data for the submesh on the VBO and IBO buffers
	u32 vertexOffset;
	u32 indexOffset;

	// Vertex Attribute Object
	std::vector<Vao> vaos;
};

struct Mesh
{
	std::vector<Submesh> submeshes;
	GLuint vertexBufferHandle;
	GLuint indexBufferHandle;
};

struct Material
{
	std::string name;
	vec3 albedo;
	vec3 emissive;
	f32 smoothness;
	u32 albedoTextureIdx;
	u32 emissiveTextureIdx;
	u32 specularTextureIdx;
	u32 normalsTextureIdx;
	u32 bumpTextureIdx;
};

struct Model
{
	u32 meshIdx;
	std::vector<u32> materialIdx;
};

struct Camera
{
	vec3 position;
	vec3 target;
	vec3 direction;
	float fov;
	float znear;
	float zfar;
};

struct Program
{
	GLuint             handle;
	std::string        filepath;
	std::string        programName;
	u64                lastWriteTimestamp;
	VertexShaderLayout vertexInputLayout;
};

struct Entity
{
	glm::mat4 worldMatrix;
	u32 modelIndex;
	u32 head;
	u32 size;
};

enum LightType
{
	LightType_Directional,
	LightType_Point
};

struct Light
{
	LightType type;
	vec3 color;
	vec3 direction;
	vec3 position;
};

struct App
{
	// Loop
	f32  deltaTime;
	bool isRunning;

	// Input
	Input input;

	// Graphics
	OpenGL_Info openglInfo;

	ivec2 displaySize;

	std::vector<Texture>  textures;
	std::vector<Program>  programs;
	std::vector<Material> materials;
	std::vector<Mesh>     meshes;
	std::vector<Model>    models;
	std::vector<Entity>   entities;
	std::vector<Light>    lights;

	// program indices
	u32 texturedGeometryProgramIdx;
	u32 texturedMeshProgramIdx;

	// texture indices
	u32 diceTexIdx;
	u32 whiteTexIdx;
	u32 blackTexIdx;
	u32 normalTexIdx;
	u32 magentaTexIdx;
	u32 patrickTexIdx;

	// model indices
	u32 patrickModel;
	u32 sphere;
	u32 plane;

	// Mode
	Mode mode;

	// Embedded geometry (in-editor simple meshes such as
	// a screen filling quad, a cube, a sphere...)
	GLuint embeddedVertices;
	GLuint embeddedElements;

	// Location of the texture uniform in the textured quad shader
	GLuint programUniformTexture;
	GLuint texturedMeshProgram_uTexture;
	GLuint texturedMeshProgram_uNormal;
	GLuint texturedMeshProgram_uAO;
	GLuint texturedMeshProgram_uEmissive;
	GLuint texturedMeshProgram_uSpecular;
	GLuint texturedMeshProgram_uRoughness;
	GLuint texturedMeshProgram_uDepth;

	// VAO object to link our screen filling quad with our textured quad shader
	GLuint vao;

	Camera camera;
	glm::mat4 worldViewProjectionMatrix;

	// buffers
	Buffer uniformBuffer;
	GLint uniformBlockAlignment;
	GLint maxUniformBufferSize;

	u32 globalParamsOffset;
	u32 globalParamsSize;

	// framebuffers
	GLuint albedoAO_attachmentHandle;
	GLuint specularRoughness_attachmentHandle;
	GLuint normals_attachmentHandle;
	GLuint emissiveLightmaps_attachmentHandle;
	GLuint position_attachmentHandle;
	GLuint depthAttachmentHandle;
	GLuint framebufferHandle;

};

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);
