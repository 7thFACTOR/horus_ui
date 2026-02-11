#include "opengl_graphics.h"
#include <string.h>

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>

#ifndef _DEBUG
#define OGL_CHECK_ERROR { char errStr[1024] = {0}; sprintf(errStr, "File: %s, line: %d", __FILE__, __LINE__); checkErrorGL(errStr); };
#else
#define OGL_CHECK_ERROR
#endif

namespace hui
{
OpenGLTexture::OpenGLTexture(u32 newWidth, u32 newHeight, Rgba32* pixels)
{
	resize(newWidth, newHeight);
	updateData(pixels);
}

OpenGLTexture::OpenGLTexture(u32 newWidth, u32 newHeight)
{
	resize(newWidth, newHeight);
}

OpenGLTexture::~OpenGLTexture()
{
	destroy();
}

void OpenGLTexture::resize(u32 newWidth, u32 newHeight)
{
	if (!handle)
	{
		glGenTextures(1, &handle);
	}

	width = newWidth;
	height = newHeight;
	glBindTexture(GL_TEXTURE_2D, handle);
	OGL_CHECK_ERROR;
	glTexImage2D(GL_TEXTURE_2D,
		0,
		GL_RGBA8,
		width, height,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		0);
	OGL_CHECK_ERROR;
}

void OpenGLTexture::updateData(Rgba32* pixels)
{
	glBindTexture(GL_TEXTURE_2D, handle);
	OGL_CHECK_ERROR;
	glTexImage2D(
		GL_TEXTURE_2D,
		0, 0,
		width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	OGL_CHECK_ERROR;
}

void OpenGLTexture::updateRectData(const Rect& rect, Rgba32* pixels)
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
	OGL_CHECK_ERROR;
	glTexSubImage2D(
		GL_TEXTURE_2D_ARRAY,
		0, rect.x, rect.y, rect.width, rect.height,
		GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	OGL_CHECK_ERROR;
}

void OpenGLTexture::destroy()
{
	if (!handle)
	{
		return;
	}

	glDeleteTextures(1, &handle);
	OGL_CHECK_ERROR;
	handle = 0;
}

OpenGLVertexBuffer::OpenGLVertexBuffer()
{}

OpenGLVertexBuffer::OpenGLVertexBuffer(u32 count, Vertex* vertices)
{
	create(count);
	updateData(vertices, 0, count);
}

OpenGLVertexBuffer::~OpenGLVertexBuffer()
{
	destroy();
}

void OpenGLVertexBuffer::create(u32 count)
{
	destroy();
	glGenBuffers(1, (GLuint*)&handle);
	OGL_CHECK_ERROR;
	resize(count);
}

void OpenGLVertexBuffer::resize(u32 count)
{
	glBindBuffer(GL_ARRAY_BUFFER, (GLuint)handle);
	OGL_CHECK_ERROR;
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(Vertex) * count,
		nullptr,
		GL_DYNAMIC_DRAW);
	OGL_CHECK_ERROR;
	this->count = count;
}

void OpenGLVertexBuffer::updateData(Vertex* vertices, u32 startVertexIndex, u32 count)
{
	glBindBuffer(GL_ARRAY_BUFFER, handle);
	OGL_CHECK_ERROR;

	u8* data = (u8*)glMapBufferRange(
		GL_ARRAY_BUFFER,
		sizeof(Vertex) * startVertexIndex,
		sizeof(Vertex) * count,
		GL_MAP_WRITE_BIT);
	OGL_CHECK_ERROR;

	if (!data)
	{
		return;
	}

	memcpy(
		data,
		&vertices[startVertexIndex],
		count * sizeof(Vertex));

	glUnmapBuffer(GL_ARRAY_BUFFER);
	OGL_CHECK_ERROR;
}

void OpenGLVertexBuffer::destroy()
{
	glDeleteBuffers(1, &handle);
	handle = 0;
	count = 0;
}

void checkErrorGL(const char* where)
{
	GLuint err = glGetError();
	std::string str;

	switch (err)
	{
	case GL_INVALID_ENUM:
		str = "GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag.";
		break;
	case GL_INVALID_VALUE:
		str = "GL_INVALID_VALUE: A numeric argument is out of range. The offending command is ignored and has no other side effect than to set the error flag.";
		break;
	case GL_INVALID_OPERATION:
		str = "GL_INVALID_OPERATION: The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag.";
		break;
	case GL_STACK_OVERFLOW:
		str = "GL_STACK_OVERFLOW: This command would cause a stack overflow. The offending command is ignored and has no other side effect than to set the error flag.";
		break;
	case GL_STACK_UNDERFLOW:
		str = "GL_STACK_UNDERFLOW: This command would cause a stack underflow. The offending command is ignored and has no other side effect than to set the error flag.";
		break;
	case GL_OUT_OF_MEMORY:
		str = "GL_OUT_OF_MEMORY: There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";
		break;
	case GL_TABLE_TOO_LARGE:
		str = "GL_TABLE_TOO_LARGE: The specified table exceeds the implementation's maximum supported table size. The offending command is ignored and has no other side effect than to set the error flag.";
		break;
	};

	if (err != GL_NO_ERROR)
	{
		printf("[%s] OpenGL: code#%d: %s\n", where, err, str.c_str());
	}
}

static Rect currentViewport;
static GLuint vertexShader = 0;
static GLuint pixelShader = 0;
static GLuint program = 0;
static OpenGLVertexBuffer vertexBuffer;

static void setSamplerValueInGpuProgram(
	GLuint program,
	GLuint tex,
	const std::string& constName,
	u32 stage)
{
	GLint loc = glGetUniformLocation(program, constName.c_str());
	OGL_CHECK_ERROR;

	if (loc == -1)
	{
		return;
	}

	glActiveTexture(GL_TEXTURE0 + stage);
	OGL_CHECK_ERROR;
	glUniform1i(loc, stage);
	OGL_CHECK_ERROR;
	glBindTexture(GL_TEXTURE_2D, tex);
	OGL_CHECK_ERROR;
}

static void setIntValueInGpuProgram(
	GLuint program,
	GLuint value,
	const std::string& constName)
{
	GLint loc = glGetUniformLocation(program, constName.c_str());
	OGL_CHECK_ERROR;

	if (loc == -1)
	{
		return;
	}

	glUniform1i(loc, value);
	OGL_CHECK_ERROR;
}

static const char* uiVertexShaderSource =
"\
#version 130\r\n\
\
in vec2 inPOSITION;\
in vec2 inTEXCOORD0;\
in uint inCOLOR;\
\
uniform mat4 mvp;\
out vec2 outTEXCOORD;\
out vec4 outCOLOR;\
\
void main()\
{\
	vec4 v = mvp * vec4(inPOSITION.x, inPOSITION.y, 0, 1);\
	gl_Position = v;\
	outTEXCOORD = inTEXCOORD0;\
	vec4 color = vec4(float(inCOLOR & uint(0x000000FF))/255.0, float((inCOLOR & uint(0x0000FF00)) >> uint(8))/255.0, float((inCOLOR & uint(0x00FF0000)) >> uint(16))/255.0, float((inCOLOR & uint(0xFF000000))>> uint(24))/255.0);\
	outCOLOR = color;\
	return;\
}\
";

static const char* uiPixelShaderSource =
"\
#version 130\r\n\
uniform sampler2D diffuseSampler;\
in vec2 outTEXCOORD;\
in vec4 outCOLOR;\
out vec4 finalCOLOR;\
\
void main()\
{\
	finalCOLOR = outCOLOR * texture2D(diffuseSampler, outTEXCOORD);\
}\
";

static void commitRenderState()
{
	glEnable(GL_BLEND);
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
	glDisable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);
}

static void setViewport(const Point& windowSize, const Rect& viewport)
{
	// our 0,0 origin is at top-left, we need to modify y
	Rect glRc = { viewport.x, windowSize.y - viewport.y - viewport.height, viewport.width, viewport.height };
	currentViewport = glRc;
	glViewport(glRc.x, glRc.y, glRc.width, glRc.height);
	OGL_CHECK_ERROR;
}

static void clearBackbuffer(const Color& color)
{
	glClearColor(color.r, color.g, color.b, color.a);
	OGL_CHECK_ERROR;
	glClear(GL_COLOR_BUFFER_BIT);
	OGL_CHECK_ERROR;
}

static void draw(Vertex* vertices, u32 vertexCount, struct RenderBatch* batches, u32 count)
{
	glUseProgram(program);
	OGL_CHECK_ERROR;

	if (vertexBuffer.count < vertexCount)
	{
		vertexBuffer.resize(vertexCount);
	}

	vertexBuffer.updateData(vertices, 0, vertexCount);

	// render the batches
	#define OGL_VBUFFER_OFFSET(i) ((void*)(i))

	for (u32 i = 0; i < count; i++)
	{
		auto& batch = batches[i];

		commitRenderState();
		setSamplerValueInGpuProgram(
			program,
			(GLuint)batch.texture,
			"diffuseSampler", 0);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		OGL_CHECK_ERROR;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		OGL_CHECK_ERROR;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		OGL_CHECK_ERROR;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		OGL_CHECK_ERROR;

		GLint loc = glGetUniformLocation((GLuint)program, "mvp");
		OGL_CHECK_ERROR;

		if (loc != -1)
		{
			GLint vp[4];
			glGetIntegerv(GL_VIEWPORT, vp);
			OGL_CHECK_ERROR;

			f32 left = 0;
			f32 right = vp[2];
			f32 top = 0;
			f32 bottom = vp[3];

			f32 m[4][4] = { 0 };

			m[0][0] = 2.0f / vp[2];
			m[0][1] = 0.0f;
			m[0][2] = 0.0f;
			m[0][3] = 0.0f;

			m[1][0] = 0.0f;
			m[1][1] = 2.0f / -vp[3];
			m[1][2] = 0.0f;
			m[1][3] = 0.0f;

			m[2][0] = 0.0f;
			m[2][1] = 0.0f;
			m[2][2] = 1.0f;
			m[2][3] = 0.0f;

			m[3][0] = -1;
			m[3][1] = 1;
			m[3][2] = 0.0f;
			m[3][3] = 1.0f;

			glUniformMatrix4fv(loc, 1, false, (GLfloat*)m);
			OGL_CHECK_ERROR;
		}
		else
		{
			glUseProgram(0);
			OGL_CHECK_ERROR;
			return;
		}

		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.handle);
		OGL_CHECK_ERROR;

		u32 stride = sizeof(Vertex);
		u32 attrLoc = 0;
		u32 offsetSum = 0;

		attrLoc = glGetAttribLocation(program, "inPOSITION");
		OGL_CHECK_ERROR;

		if (attrLoc == ~0)
		{
			return;
		}

		glEnableVertexAttribArray(attrLoc);
		OGL_CHECK_ERROR;
		glVertexAttribPointer(attrLoc, 2, GL_FLOAT, GL_FALSE, stride, OGL_VBUFFER_OFFSET(offsetSum));
		OGL_CHECK_ERROR;

		if (glVertexAttribDivisor) glVertexAttribDivisor(attrLoc, 0);
		OGL_CHECK_ERROR;
		offsetSum += sizeof(f32) * 2;

		attrLoc = glGetAttribLocation(program, "inTEXCOORD0");
		OGL_CHECK_ERROR;
		glEnableVertexAttribArray(attrLoc);
		OGL_CHECK_ERROR;
		glVertexAttribPointer(attrLoc, 2, GL_FLOAT, GL_FALSE, stride, OGL_VBUFFER_OFFSET(offsetSum));
		OGL_CHECK_ERROR;

		if (glVertexAttribDivisor) glVertexAttribDivisor(attrLoc, 0);
		OGL_CHECK_ERROR;
		offsetSum += sizeof(f32) * 2;

		attrLoc = glGetAttribLocation(program, "inCOLOR");
		OGL_CHECK_ERROR;
		glEnableVertexAttribArray(attrLoc);
		OGL_CHECK_ERROR;
		glVertexAttribIPointer(attrLoc, 1, GL_UNSIGNED_INT, stride, OGL_VBUFFER_OFFSET(offsetSum));
		OGL_CHECK_ERROR;

		if (glVertexAttribDivisor) glVertexAttribDivisor(attrLoc, 0);
		OGL_CHECK_ERROR;
		offsetSum += sizeof(u32);

		int primType = GL_TRIANGLES;

		if (batch.primitiveType == RenderBatch::PrimitiveType::TriangleStrip)
		{
			primType = GL_TRIANGLE_STRIP;
		}
		else if (batch.primitiveType == RenderBatch::PrimitiveType::TriangleFan)
		{
			primType = GL_TRIANGLE_FAN;
		}

		glDrawArrays(primType, batch.startVertexIndex, batch.vertexCount);
		OGL_CHECK_ERROR;
	}

	glUseProgram(0);
	OGL_CHECK_ERROR;
}

bool initOpenGL(Services& services)
{
	GLchar errorLog[1024] = { 0 };

	printf("Initializing HorusUI OpenGL provider...\n");
	program = glCreateProgram();

	pixelShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(pixelShader, 1, &uiPixelShaderSource, nullptr);
	OGL_CHECK_ERROR;
	glCompileShader(pixelShader);
	OGL_CHECK_ERROR;
	glAttachShader((GLuint)program, pixelShader);
	OGL_CHECK_ERROR;

	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &uiVertexShaderSource, nullptr);
	OGL_CHECK_ERROR;
	glCompileShader(vertexShader);
	OGL_CHECK_ERROR;
	glAttachShader((GLuint)program, vertexShader);
	OGL_CHECK_ERROR;

	glLinkProgram((GLuint)program);
	OGL_CHECK_ERROR;

	{
		glGetProgramInfoLog((GLuint)program, 1024, NULL, errorLog);
		OGL_CHECK_ERROR;

		if (strcmp(errorLog, ""))
		{
			printf("Linking program: %s\n", errorLog);
		}
	}

	if (!glIsProgram((GLuint)program))
	{
		printf("Program ID:%d is not a valid OpenGL program\n", program);
	}
	OGL_CHECK_ERROR;

	GLint err;
	glGetShaderiv(pixelShader, GL_COMPILE_STATUS, &err);
	OGL_CHECK_ERROR;

	if (!err)
	{
		glGetShaderInfoLog((GLuint)pixelShader, 1024, NULL, errorLog);
		OGL_CHECK_ERROR;
		printf("Error validating pixel shader: '%s'\n", errorLog);
		return false;
	}

	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &err);
	OGL_CHECK_ERROR;

	if (!err)
	{
		glGetShaderInfoLog((GLuint)vertexShader, 1024, NULL, errorLog);
		OGL_CHECK_ERROR;
		printf("Error validating vertex shader: '%s'\n", errorLog);
		return false;
	}

	glValidateProgram((GLuint)program);
	OGL_CHECK_ERROR;

	GLint success = GL_FALSE;
	glGetProgramiv((GLuint)program, GL_VALIDATE_STATUS, &success);
	OGL_CHECK_ERROR;

	if (success == GL_FALSE)
	{
		glGetProgramInfoLog((GLuint)program, 1024, NULL, errorLog);
		OGL_CHECK_ERROR;
		printf("Error validating program: '%s'\n", errorLog);
		return false;
	}

	GLint numAttrs;
	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &numAttrs);
	OGL_CHECK_ERROR;

	for (int n = 0; n < numAttrs; n++)
	{
		GLsizei len;
		GLint size;
		GLenum type;
		GLchar name[100];
		glGetActiveAttrib(program, n, 100, &len, &size, &type, name);
		OGL_CHECK_ERROR;
	}

	vertexBuffer.create(10000);

	services.setViewport = setViewport;
	services.clearBackbuffer = clearBackbuffer;
	services.draw = draw;

	return true;
}

void shutdownOpenGL(Services& services)
{
	glDeleteShader(vertexShader);
	glDeleteShader(pixelShader);
	glDeleteProgram(program);

	services.setViewport = nullptr;
	services.clearBackbuffer = nullptr;
	services.draw = nullptr;
}

}