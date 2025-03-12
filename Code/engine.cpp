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

void InitLoadTextures(App* app)
{
	app->diceTexIdx = LoadTexture2D(app, "dice.png");
	app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
	app->blackTexIdx = LoadTexture2D(app, "color_black.png");
	app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
	app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");
}

void TextureQuadMode(App* app)
{
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, app->displaySize.x, app->displaySize.y);

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

void Init(App* app)
{
	GetOpenGLContext(app);

	struct VertexV3V2
	{
		glm::vec3 pos;
		glm::vec2 uv;
	};

	const VertexV3V2 vertices[] = {
		{ glm::vec3(-0.5, -0.5, 0.0),  glm::vec2(0.0, 0.0) },
		{ glm::vec3(0.5, -0.5, 0.0),  glm::vec2(1.0, 0.0) },
		{ glm::vec3(0.5,  0.5, 0.0),  glm::vec2(1.0, 1.0) },
		{ glm::vec3(-0.5,  0.5, 0.0),  glm::vec2(0.0, 1.0) },
	};

	const u16 indices[] = { 0,1,2,0,2,3 };

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

	// - textures
	InitLoadTextures(app);

	app->mode = Mode_TexturedQuad;
}

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
		for (const auto& extension : app->openglInfo.extensions)
		{
			ImGui::Text("%s", extension.c_str());
		}
	}
	ImGui::End();
}

void Gui(App* app)
{
	InfoWindow(app);
}

void Update(App* app)
{
	// You can handle app->input keyboard/mouse here
}

void Render(App* app)
{
	switch (app->mode)
	{
	case Mode_TexturedQuad:
	{
		// TODO: Draw your textured quad here!
		// - clear the framebuffer
		// - set the viewport
		// - set the blending state
		// - bind the texture into unit 0
		// - bind the program 
		//   (...and make its texture sample from unit 0)
		// - bind the vao
		// - glDrawElements() !!!

		TextureQuadMode(app);
	}
	break;

	default:;
	}
}
