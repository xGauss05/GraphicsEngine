//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
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

struct Program
{
	GLuint             handle;
	std::string        filepath;
	std::string        programName;
	u64                lastWriteTimestamp; // What is this for?
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

};

struct VertexV3V2
{
	glm::vec3 pos;
	glm::vec2 uv;
};

const VertexV3V2 vertices[] = 
{
	{ glm::vec3(-0.5, -0.5, 0.0),  glm::vec2(0.0, 0.0) },
	{ glm::vec3(0.5, -0.5, 0.0),   glm::vec2(1.0, 0.0) },
	{ glm::vec3(0.5,  0.5, 0.0),   glm::vec2(1.0, 1.0) },
	{ glm::vec3(-0.5,  0.5, 0.0),  glm::vec2(0.0, 1.0) },
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

	// program indices
	u32 texturedGeometryProgramIdx;
	u32 texturedMeshProgramIdx;

	// texture indices
	u32 diceTexIdx;
	u32 whiteTexIdx;
	u32 blackTexIdx;
	u32 normalTexIdx;
	u32 magentaTexIdx;

	// Mode
	Mode mode;

	// Embedded geometry (in-editor simple meshes such as
	// a screen filling quad, a cube, a sphere...)
	GLuint embeddedVertices;
	GLuint embeddedElements;

	// Location of the texture uniform in the textured quad shader
	GLuint programUniformTexture;

	// VAO object to link our screen filling quad with our textured quad shader
	GLuint vao;


};

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);
