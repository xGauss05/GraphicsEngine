//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include "colors.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>

// Open GL functions
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
	// To check later whether or not the file was modified since it was loaded
	program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);

	int attributeCount = 0;
	glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);

	GLint maxAttributeNameLength = 0;
	glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxAttributeNameLength);

	for (u32 i = 0; i < attributeCount; i++)
	{
		std::string attributeName(maxAttributeNameLength, '\0');
		GLsizei attributeNameLength;
		GLint attributeSize;
		GLenum attributeType;

		glGetActiveAttrib(program.handle, i, maxAttributeNameLength, &attributeNameLength, &attributeSize, &attributeType, &attributeName[0]);
		attributeName.resize(attributeNameLength);

		u8 attributeLocation = glGetAttribLocation(program.handle, attributeName.c_str());
		u8 componentCount = (u8)(attributeType == GL_FLOAT_VEC3 ? 3 : (attributeType == GL_FLOAT_VEC2 ? 2 : 1));

		program.vertexInputLayout.vsAttributes.push_back({ attributeLocation, componentCount });
	}

	app->programs.push_back(program);

	return app->programs.size() - 1;
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

void GetOpenGLContext(App* app)
{
	app->openglInfo.version = (char*)glGetString(GL_VERSION);
	app->openglInfo.renderer = (char*)glGetString(GL_RENDERER);
	app->openglInfo.vendor = (char*)glGetString(GL_VENDOR);
	app->openglInfo.shading_language_version = (char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

	GLint n_extensions;
	glGetIntegerv(GL_NUM_EXTENSIONS, &n_extensions);

	GLint numExtensions = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

	for (GLint i = 0; i < numExtensions; ++i)
	{
		const GLubyte* extension = glGetStringi(GL_EXTENSIONS, GLuint(i));
		if (extension)
		{
			app->openglInfo.extensions.push_back(reinterpret_cast<const char*>(extension));
		}
	}
}

void OnGlError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
		return;

	ELOG("OpenGL debug message: %s", message);

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:				ELOG(" - source: GL_DEBUG_SOURCE_API"); break; // Calls to the OpenGL API
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:		ELOG(" - source: GL_DEBUG_SOURCE_WINDOW_SYSTEM"); break; // Calls to a window-system API
	case GL_DEBUG_SOURCE_SHADER_COMPILER:	ELOG(" - source: GL_DEBUG_SOURCE_SHADER_COMPILER"); break; // A compiler for a shading language
	case GL_DEBUG_SOURCE_THIRD_PARTY:		ELOG(" - source: GL_DEBUG_SOURCE_THIRD_PARTY"); break; // An application associated with OpenGL
	case GL_DEBUG_SOURCE_APPLICATION:		ELOG(" - source: GL_DEBUG_SOURCE_APPLICATION"); break; // Generated by the user of this applicat
	case GL_DEBUG_SOURCE_OTHER:				ELOG(" - source: GL_DEBUG_SOURCE_OTHER"); break; // Some source that isn't one of these
	}

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               ELOG(" - type: GL_DEBUG_TYPE_ERROR"); break; // An error, typically from the API
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: ELOG(" - type: GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR"); break; // Some behavior marked deprecated l
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  ELOG(" - type: GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR"); break; // Something has invoked undefined b
	case GL_DEBUG_TYPE_PORTABILITY:         ELOG(" - type: GL_DEBUG_TYPE_PORTABILITY"); break; // Some functionality the user relies upon
	case GL_DEBUG_TYPE_PERFORMANCE:         ELOG(" - type: GL_DEBUG_TYPE_PERFORMANCE"); break; // Code has triggered possible performance
	case GL_DEBUG_TYPE_MARKER:              ELOG(" - type: GL_DEBUG_TYPE_MARKER"); break; // Command stream annotation
	case GL_DEBUG_TYPE_PUSH_GROUP:          ELOG(" - type: GL_DEBUG_TYPE_PUSH_GROUP"); break; // Group pushing
	case GL_DEBUG_TYPE_POP_GROUP:           ELOG(" - type: GL_DEBUG_TYPE_POP_GROUP"); break; // Group popping
	case GL_DEBUG_TYPE_OTHER:               ELOG(" - type: GL_DEBUG_TYPE_OTHER"); break; // Some type that isn't one of these
	}

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:			ELOG(" - severity: GL_DEBUG_SEVERITY_HIGH"); break; // All OpenGL Errors, shader compilation/link
	case GL_DEBUG_SEVERITY_MEDIUM:			ELOG(" - severity: GL_DEBUG_SEVERITY_MEDIUM"); break; // Major performance warnings, shader compil
	case GL_DEBUG_SEVERITY_LOW:				ELOG(" - severity: GL_DEBUG_SEVERITY_LOW"); break; // Redundant state change performance warning,
	case GL_DEBUG_SEVERITY_NOTIFICATION:	ELOG(" - severity: GL_DEBUG_SEVERITY_NOTIFICATION"); break; // Anything that isn't an error or per
	}
}

// Assimp functions
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
	vertexBufferLayout.vbAttributes.push_back(VertexBufferAttribute{ 0, 3, 0 }); // 3D positions
	vertexBufferLayout.vbAttributes.push_back(VertexBufferAttribute{ 1, 3, 3 * sizeof(float) }); // tex coords
	vertexBufferLayout.stride = 6 * sizeof(float);
	if (hasTexCoords)
	{
		vertexBufferLayout.vbAttributes.push_back(VertexBufferAttribute{ 2, 2, vertexBufferLayout.stride });
		vertexBufferLayout.stride += 2 * sizeof(float);
	}
	if (hasTangentSpace)
	{
		vertexBufferLayout.vbAttributes.push_back(VertexBufferAttribute{ 3, 3, vertexBufferLayout.stride });
		vertexBufferLayout.stride += 3 * sizeof(float);

		vertexBufferLayout.vbAttributes.push_back(VertexBufferAttribute{ 4, 3, vertexBufferLayout.stride });
		vertexBufferLayout.stride += 3 * sizeof(float);
	}

	// add the submesh into the mesh
	Submesh submesh = {};
	submesh.vbLayout = vertexBufferLayout;
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

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
	Submesh& submesh = mesh.submeshes[submeshIndex];

	// Try finding a vao for this submesh/program
	for (u32 i = 0; i < (u32)submesh.vaos.size(); i++)
	{
		if (submesh.vaos[i].programHandle == program.handle)
			return submesh.vaos[i].handle;
	}

	GLuint vaoHandle = 0;

	// Create a new vao for this submesh/program
	{
		glGenVertexArrays(1, &vaoHandle);
		glBindVertexArray(vaoHandle);

		glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

		// We have to link all vertex inputs attributes to attributes in the vertex buffer
		for (u32 i = 0; i < program.vertexInputLayout.vsAttributes.size(); i++)
		{
			bool attributeWasLinked = false;

			for (u32 j = 0; j < submesh.vbLayout.vbAttributes.size(); j++)
			{
				if (program.vertexInputLayout.vsAttributes[i].location == submesh.vbLayout.vbAttributes[j].location)
				{
					const u32 index = submesh.vbLayout.vbAttributes[j].location;
					const u32 ncomp = submesh.vbLayout.vbAttributes[j].componentCount;
					const u32 offset = submesh.vbLayout.vbAttributes[j].offset + submesh.vertexOffset; // attribute offset + vertex offset
					const u32 stride = submesh.vbLayout.stride;
					glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
					glEnableVertexAttribArray(index);

					attributeWasLinked = true;
					break;
				}
			}

			assert(attributeWasLinked); // The submesh should provide an attribute for each vertex inputs
		}

		glBindVertexArray(0);
	}

	// Store it in the list for this submesh
	Vao vao = { vaoHandle, program.handle };
	submesh.vaos.push_back(vao);

	return vaoHandle;
}

// GLM functions
glm::mat4 TransformScale(const vec3& scaleFactors)
{
	glm::mat4 transform = glm::scale(scaleFactors);
	return transform;
}

glm::mat4 TransformPositionScale(const vec3& pos, const vec3& scaleFactors)
{
	glm::mat4 transform = glm::translate(pos);
	transform = scale(transform, scaleFactors);
	return transform;
}

// Utilities
void ChangeAppMode(App* app, Mode mode)
{
	if (app->mode != mode) app->mode = mode;
}

// Camera functions
void CameraMovement(App* app)
{
	float camSpeed = 0.8f;

	// Position
	if (app->input.keys[K_W]) { app->camera.position.y += app->deltaTime * camSpeed; }
	if (app->input.keys[K_A]) { app->camera.position.x -= app->deltaTime * camSpeed; }
	if (app->input.keys[K_S]) { app->camera.position.y -= app->deltaTime * camSpeed; }
	if (app->input.keys[K_D]) { app->camera.position.x += app->deltaTime * camSpeed; }

	// Target
	if (app->input.keys[K_I]) { app->camera.target.y += app->deltaTime * camSpeed; }
	if (app->input.keys[K_J]) { app->camera.target.x -= app->deltaTime * camSpeed; }
	if (app->input.keys[K_K]) { app->camera.target.y -= app->deltaTime * camSpeed; }
	if (app->input.keys[K_L]) { app->camera.target.x += app->deltaTime * camSpeed; }

}

// Init functions
void InitLoadTextures(App* app)
{

	app->diceTexIdx = LoadTexture2D(app, "dice.png");
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		// after loading texture, err is 500: 
		// GL_INVALID_ENUM error, which indicates that an unacceptable value was specified for an 
		// enumerated argument in an OpenGL function call
		// ignore?
		ELOG("OpenGL Error: %x", err);
	}
	app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
	app->blackTexIdx = LoadTexture2D(app, "color_black.png");
	app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
	app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");

}

void InitQuadMode(App* app)
{
	// Geometry
	// - vertex buffers
	glGenBuffers(1, &app->embeddedVertices);
	glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// - element/index buffers
	glGenBuffers(1, &app->embeddedElements);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Attribute (VAO initialization)
	// - vaos
	glGenVertexArrays(1, &app->vao);
	glBindVertexArray(app->vao);
	glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
	glBindVertexArray(0);

	// - programs (and retrieve uniform indices)
	app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
	Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];

	app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");

}

void InitMeshMode(App* app)
{
	app->patrickModel = LoadModel(app, "Patrick/Patrick.obj");

	// - programs
	app->texturedMeshProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_TEXTURED_MESH");
	Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];

	app->texturedMeshProgram_uTexture = glGetUniformLocation(texturedMeshProgram.handle, "uTexture");
	app->texturedMeshProgram_uNormal = glGetUniformLocation(texturedMeshProgram.handle, "uNormal");
	app->texturedMeshProgram_uAO = glGetUniformLocation(texturedMeshProgram.handle, "uAO");
	app->texturedMeshProgram_uEmissive = glGetUniformLocation(texturedMeshProgram.handle, "uEmissive");
	app->texturedMeshProgram_uSpecular = glGetUniformLocation(texturedMeshProgram.handle, "uSpecular");
	app->texturedMeshProgram_uRoughness = glGetUniformLocation(texturedMeshProgram.handle, "uRoughness");


	Entity en1 = { TransformPositionScale(vec3(2.0f, 1.5f, -2.0f),  vec3(0.45f)), app->patrickModel };
	Entity en2 = { TransformPositionScale(vec3(-2.0f, 1.5f, -2.0f), vec3(0.45f)), app->patrickModel };
	Entity en3 = { TransformPositionScale(vec3(0.0f, 1.5f, -2.0f),  vec3(0.45f)), app->patrickModel };

	app->entities.push_back(en1);
	app->entities.push_back(en2);
	app->entities.push_back(en3);

	Light li1 = { LightType_Directional, Colors::Yellow, vec3(1.0), vec3(1.0) };
	Light li2 = { LightType_Point, Colors::Magenta, vec3(1.0), vec3(1.0) };
	Light li3 = { LightType_Directional, Colors::Cyan, vec3(1.0), vec3(1.0) };

	app->lights.push_back(li1);
	app->lights.push_back(li2);
	app->lights.push_back(li3);
}

void InitFramebuffer(App* app)
{
	// Framebuffer
	glGenTextures(1, &app->albedoAO_attachmentHandle);
	glBindTexture(GL_TEXTURE_2D, app->albedoAO_attachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &app->specularRoughness_attachmentHandle);
	glBindTexture(GL_TEXTURE_2D, app->specularRoughness_attachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &app->normals_attachmentHandle);
	glBindTexture(GL_TEXTURE_2D, app->normals_attachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &app->emissiveLightmaps_attachmentHandle);
	glBindTexture(GL_TEXTURE_2D, app->emissiveLightmaps_attachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &app->depthAttachmentHandle);
	glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &app->position_attachmentHandle);
	glBindTexture(GL_TEXTURE_2D, app->position_attachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &app->framebufferHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->albedoAO_attachmentHandle, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->specularRoughness_attachmentHandle, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, app->normals_attachmentHandle, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, app->emissiveLightmaps_attachmentHandle, 0);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->depthAttachmentHandle, 0);

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
		default:											ELOG("Unknown framebuffer status error");
		}
	}

	//glDrawBuffers(1, &app->albedoAO_attachmentHandle);

	GLenum drawBuffers[] = {
GL_COLOR_ATTACHMENT0,
GL_COLOR_ATTACHMENT1,
GL_COLOR_ATTACHMENT2,
GL_COLOR_ATTACHMENT3,
GL_COLOR_ATTACHMENT4,
	};

	glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Init(App* app)
{
	if (GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 3))
	{
		glDebugMessageCallback(OnGlError, app);
	}

	glEnable(GL_DEPTH_TEST);

	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);

	app->uniformBuffer = CreateConstantBuffer(app->maxUniformBufferSize);

	// Camera init
	app->camera = {};
	app->camera.position = glm::vec3(0.0f, 0.0f, 3.0f);
	app->camera.target = glm::vec3(0.0f);
	app->camera.direction = glm::normalize(app->camera.position - app->camera.target);
	app->camera.znear = 0.1f;
	app->camera.zfar = 1000.0f;
	app->camera.fov = 60.0f;

	CameraMovement(app);

	GetOpenGLContext(app);

	InitLoadTextures(app);
	InitQuadMode(app);

	InitMeshMode(app);

	InitFramebuffer(app);

	app->mode = Mode_Mesh; // default mode
}

// GUI functions
void InfoWindow(App* app)
{
	ImGui::Begin("Info");
	ImGui::Text("FPS: %f", 1.0f / app->deltaTime);
	ImGui::Text("Version: %s", app->openglInfo.version);
	ImGui::Text("Renderer: %s", app->openglInfo.renderer);
	ImGui::Text("Vendor: %s", app->openglInfo.vendor);
	ImGui::Text("Shading Language Version: %s", app->openglInfo.shading_language_version);

	if (ImGui::CollapsingHeader("Extensions", ImGuiTreeNodeFlags_None))
	{
		ImGui::BeginChild("ExtensionsList", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar); // Área de scroll con 150px de alto

		for (const auto& extension : app->openglInfo.extensions)
		{
			ImGui::Text("%s", extension.c_str());
		}

		ImGui::EndChild();
	}

	ImGui::End();
}

void RenderModeWindow(App* app)
{
	ImGui::Begin("Render Mode");

	if (ImGui::Button("Textured Quad"))
	{
		ChangeAppMode(app, Mode_TexturedQuad);
	}

	if (ImGui::Button("Mesh"))
	{
		ChangeAppMode(app, Mode_Mesh);
	}

	if (ImGui::Button("Framebuffer"))
	{
		ChangeAppMode(app, Mode_Framebuffer);
	}

	if (ImGui::Button("Albedo"))
	{
		ChangeAppMode(app, Mode_Albedo);
	}

	if (ImGui::Button("Normal"))
	{
		ChangeAppMode(app, Mode_Normal);
	}

	if (ImGui::Button("Position"))
	{
		ChangeAppMode(app, Mode_Position);
	}

	ImGui::End();
}

// Gui -- where ImGui windows draw stuff
void Gui(App* app)
{
	InfoWindow(app);
	RenderModeWindow(app);
}

void HotReload(App* app)
{
	// Check timestamp / reload
	for (u64 i = 0; i < app->programs.size(); i++)
	{
		Program& program = app->programs[i];
		u64 currentTimestamp = GetFileLastWriteTimestamp(program.filepath.c_str());
		if (currentTimestamp > program.lastWriteTimestamp)
		{
			glDeleteProgram(program.handle);
			String progSource = ReadTextFile(program.filepath.c_str());
			const char* progName = program.programName.c_str();

			program.handle = CreateProgramFromSource(progSource, progName);
			program.lastWriteTimestamp = currentTimestamp;
		}
	}
}

// Update -- where input, hot reload, and buffer ordering are
void Update(App* app)
{
	// You can handle app->input keyboard/mouse here
	if (app->input.keys[K_ESCAPE]) app->isRunning = false;
	if (app->input.keys[K_1]) ChangeAppMode(app, Mode_TexturedQuad);
	if (app->input.keys[K_2]) ChangeAppMode(app, Mode_Mesh);
	if (app->input.keys[K_3]) ChangeAppMode(app, Mode_Framebuffer);
	if (app->input.keys[K_4]) ChangeAppMode(app, Mode_Normal);
	if (app->input.keys[K_5]) ChangeAppMode(app, Mode_Depth);

	HotReload(app);

	CameraMovement(app);

	float aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
	vec3 up = vec3{ 0.0f, 1.0f, 0.0f };
	glm::mat4 projection = glm::perspective(glm::radians(app->camera.fov), aspectRatio, app->camera.znear, app->camera.zfar);
	glm::mat4 view = glm::lookAt(app->camera.position, app->camera.target, up); // eye, center, up

	// Push data into the buffer ordered according to the uniform block
	MapBuffer(app->uniformBuffer, GL_WRITE_ONLY);

	app->globalParamsOffset = app->uniformBuffer.head;
	PushVec3(app->uniformBuffer, app->camera.position);
	PushUInt(app->uniformBuffer, app->lights.size());

	for (Light& l : app->lights)
	{
		AlignHead(app->uniformBuffer, sizeof(vec4));

		PushUInt(app->uniformBuffer, l.type);
		PushVec3(app->uniformBuffer, l.color);
		PushVec3(app->uniformBuffer, l.direction);
		PushVec3(app->uniformBuffer, l.position);
	}

	app->globalParamsSize = app->uniformBuffer.head - app->globalParamsOffset; // It's doing -0

	for (Entity& e : app->entities)
	{
		AlignHead(app->uniformBuffer, app->uniformBlockAlignment);
		app->worldViewProjectionMatrix = projection * view * e.worldMatrix;

		e.head = app->uniformBuffer.head;

		PushMat4(app->uniformBuffer, e.worldMatrix);
		PushMat4(app->uniformBuffer, app->worldViewProjectionMatrix);

		e.size = app->uniformBuffer.head - e.head;

	}

	UnmapBuffer(app->uniformBuffer);
}

// Render functions
void RenderQuadMode(App* app)
{
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
	glUseProgram(programTexturedGeometry.handle); //bind shader
	glBindVertexArray(app->vao);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUniform1i(app->programUniformTexture, 0);
	glActiveTexture(GL_TEXTURE0);
	GLuint textureHandle = app->textures[app->diceTexIdx].handle;
	glBindTexture(GL_TEXTURE_2D, textureHandle);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

	glBindVertexArray(0);
	glUseProgram(0);
}

void RenderMeshMode(App* app)
{
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, app->displaySize.x, app->displaySize.y);

	Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
	glUseProgram(texturedMeshProgram.handle);

	glBindBufferRange(GL_UNIFORM_BUFFER, 0, app->uniformBuffer.handle, app->globalParamsOffset, app->globalParamsSize);

	for (const Entity& e : app->entities)
	{
		Model& model = app->models[e.modelIndex];
		Mesh& mesh = app->meshes[model.meshIdx];

		// Binding 1
		glBindBufferRange(GL_UNIFORM_BUFFER, 1, app->uniformBuffer.handle, e.head, e.size);

		for (u32 i = 0; i < mesh.submeshes.size(); i++)
		{
			GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
			glBindVertexArray(vao);

			u32 subMeshMaterialIdx = model.materialIdx[i];
			Material& submeshMaterial = app->materials[subMeshMaterialIdx];

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
			glUniform1i(app->texturedMeshProgram_uTexture, 0);

			Submesh& submesh = mesh.submeshes[i];
			glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
		}

	}

	glBindVertexArray(0);
	glUseProgram(0);
}

void RenderFramebufferMode(App* app)
{

	glEnable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);


	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, app->displaySize.x, app->displaySize.y);

	RenderMeshMode(app);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
	glUseProgram(programTexturedGeometry.handle); //bind shader
	glBindVertexArray(app->vao);

	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUniform1i(app->programUniformTexture, 0);

	// Albedo & Ambient Occlusion
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, app->albedoAO_attachmentHandle);

	// Specular & Roughness
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, app->specularRoughness_attachmentHandle);

	// Normals
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, app->normals_attachmentHandle);

	// Emissive & Lightmaps
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, app->emissiveLightmaps_attachmentHandle);

	// Position
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, app->position_attachmentHandle);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

void RenderAlbedoMode(App* app)
{
	glEnable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, app->displaySize.x, app->displaySize.y);

	RenderMeshMode(app);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
	glUseProgram(programTexturedGeometry.handle); //bind shader
	glBindVertexArray(app->vao);

	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUniform1i(app->programUniformTexture, 0);

	// Albedo & Ambient Occlusion
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, app->albedoAO_attachmentHandle);

	// Specular & Roughness
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, app->specularRoughness_attachmentHandle);

	// Normals
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, app->normals_attachmentHandle);

	// Emissive & Lightmaps
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, app->emissiveLightmaps_attachmentHandle);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

void RenderNormalMode(App* app)
{
	glEnable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, app->displaySize.x, app->displaySize.y);

	RenderMeshMode(app);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
	glUseProgram(programTexturedGeometry.handle); //bind shader
	glBindVertexArray(app->vao);

	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUniform1i(app->programUniformTexture, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, app->normals_attachmentHandle);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

void RenderPositionMode(App* app)
{
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);
	glUseProgram(app->programs[app->texturedGeometryProgramIdx].handle);
	glBindVertexArray(app->vao);

	glUniform1i(app->programUniformTexture, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, app->position_attachmentHandle);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

void RenderDepthMode(App* app)
{
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(app->programs[app->texturedGeometryProgramIdx].handle); // Asumiendo que tienes un shader para visualizar depth
	glBindVertexArray(app->vao);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);

	glUniform1i(app->programUniformTexture, 0);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

void Render(App* app)
{
	switch (app->mode)
	{
	case Mode_TexturedQuad:
		RenderQuadMode(app);
		break;

	case Mode_Mesh:
		RenderMeshMode(app);
		break;

	case Mode_Framebuffer:
		RenderFramebufferMode(app);
		break;

	case Mode_Albedo:
		RenderAlbedoMode(app);
		break;

	case Mode_Normal:
		RenderNormalMode(app);
		break;

	case Mode_Position:
		RenderPositionMode(app);
		break;

	case Mode_Depth:
		RenderDepthMode(app);
		break;

	default:
		break;
	}

	glBindVertexArray(0);
	glUseProgram(0);
}

