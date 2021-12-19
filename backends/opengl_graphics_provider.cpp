#include "opengl_graphics.h"
#include "horus.h"
#include "opengl_texture_array.h"
#include "opengl_vertex_buffer.h"
#include <string.h>

namespace hui
{
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
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
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
in uint inTEXINDEX;\
\
uniform mat4 mvp;\
out vec2 outTEXCOORD;\
out vec4 outCOLOR;\
flat out uint outTEXINDEX;\
\
void main()\
{\
	vec4 v = mvp * vec4(inPOSITION.x, inPOSITION.y, 0, 1);\
	gl_Position = v;\
	outTEXCOORD = inTEXCOORD0;\
	vec4 color = vec4(float(inCOLOR & uint(0x000000FF))/255.0, float((inCOLOR & uint(0x0000FF00)) >> uint(8))/255.0, float((inCOLOR & uint(0x00FF0000)) >> uint(16))/255.0, float((inCOLOR & uint(0xFF000000))>> uint(24))/255.0);\
	outCOLOR = color;\
	outTEXINDEX = inTEXINDEX;\
	return;\
}\
";

static const char* uiPixelShaderSource =
"\
#version 130\r\n\
#extension GL_EXT_texture_array : enable\r\n\
uniform sampler2DArray diffuseSampler;\
\
in vec2 outTEXCOORD;\
in vec4 outCOLOR;\
flat in uint outTEXINDEX;\
out vec4 finalCOLOR;\
\
void main()\
{\
	finalCOLOR = outCOLOR * texture2DArray(diffuseSampler, vec3(outTEXCOORD, float(outTEXINDEX)));\
}\
";

OpenGLGraphicsProvider::OpenGLGraphicsProvider()
{}

OpenGLGraphicsProvider::~OpenGLGraphicsProvider()
{}

bool OpenGLGraphicsProvider::initialize()
{
	GLenum errGlew = 0;
	glewExperimental = GL_TRUE;
	errGlew = glewInit();

	printf("Initializing HorusUI OpenGL provider...\n");

	if (errGlew != GLEW_OK)
	{
		printf("FATAL ERROR: Cannot initialize GLEW. Error: %s\n", glewGetErrorString(errGlew));
		return false;
	}

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
		GLchar errorLog[1024] = { 0 };
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
		GLchar errorLog[1024] = { 0 };
		glGetShaderInfoLog((GLuint)pixelShader, 1024, NULL, errorLog);
		OGL_CHECK_ERROR;
		printf("Error validating pixel shader: '%s'\n", errorLog);
		return false;
	}

	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &err);
	OGL_CHECK_ERROR;

	if (!err)
	{
		GLchar errorLog[1024] = { 0 };
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
		GLchar errorLog[1024] = { 0 };
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

	return true;
}

void OpenGLGraphicsProvider::shutdown()
{
	glDeleteShader(vertexShader);
	glDeleteShader(pixelShader);
	glDeleteProgram(program);
}

TextureArray* OpenGLGraphicsProvider::createTextureArray()
{
	return (TextureArray*)new OpenGLTextureArray();
}

VertexBuffer* OpenGLGraphicsProvider::createVertexBuffer()
{
	return (VertexBuffer*)new OpenGLVertexBuffer();
}

HGraphicsApiRenderTarget OpenGLGraphicsProvider::createRenderTarget(u32 width, u32 height)
{
	OpenGLRenderTarget* rt = new OpenGLRenderTarget();
	GLuint fb;
	GLuint tex;

	rt->width = width;
	rt->height = height;
	glGenFramebuffers(1, &fb);
	OGL_CHECK_ERROR;
	glGenTextures(1, &tex);
	OGL_CHECK_ERROR;
	glBindTexture(GL_TEXTURE_2D, tex);
	OGL_CHECK_ERROR;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	OGL_CHECK_ERROR;
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
	OGL_CHECK_ERROR;
	rt->frameBuffer = fb;
	rt->texture = tex;

	return rt;
}

void OpenGLGraphicsProvider::destroyRenderTarget(HGraphicsApiRenderTarget rt)
{
	if (!rt)
	{
		return;
	}

	OpenGLRenderTarget* oglRt = (OpenGLRenderTarget*)rt;

	GLuint tex = (GLuint)oglRt->texture;
	GLuint fb = (GLuint)oglRt->frameBuffer;

	glDeleteFramebuffers(1, &fb);
	OGL_CHECK_ERROR;
	glDeleteTextures(1, &tex);
	OGL_CHECK_ERROR;

	delete rt;
}

void OpenGLGraphicsProvider::setRenderTarget(HGraphicsApiRenderTarget rt)
{
	OpenGLRenderTarget* oglRt = (OpenGLRenderTarget*)rt;
	glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)oglRt->frameBuffer);
	OGL_CHECK_ERROR;
}

void OpenGLGraphicsProvider::commitRenderState()
{
	glEnable(GL_BLEND);
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
	glDisable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);
}

void OpenGLGraphicsProvider::setViewport(const Point& windowSize, const Rect& viewport)
{
	Rect glRc = { viewport.x, windowSize.y - viewport.y - viewport.height, viewport.width, viewport.height };
	currentViewport = glRc;
	glViewport(glRc.x, glRc.y, glRc.width, glRc.height);
	OGL_CHECK_ERROR;
}

void OpenGLGraphicsProvider::clear(const Color& color)
{
	glClearColor(color.r, color.g, color.b, color.a);
	OGL_CHECK_ERROR;
	glClear(GL_COLOR_BUFFER_BIT);
	OGL_CHECK_ERROR;
}

void OpenGLGraphicsProvider::draw(RenderBatch* batches, u32 count)
{
	glUseProgram(program);
	OGL_CHECK_ERROR;

	// render the batches
#define OGL_VBUFFER_OFFSET(i) ((void*)(i))

	for (u32 i = 0; i < count; i++)
	{
		auto& batch = batches[i];

		commitRenderState();
		setSamplerValueInGpuProgram(
			program,
			(uintptr_t)batch.textureArray->getHandle(),
			"diffuseSampler", 0);

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		OGL_CHECK_ERROR;
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		OGL_CHECK_ERROR;
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		OGL_CHECK_ERROR;
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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

			m[0][0] = 2.0f / currentViewport.width;
			m[0][1] = 0.0f;
			m[0][2] = 0.0f;
			m[0][3] = 0.0f;

			m[1][0] = 0.0f;
			m[1][1] = 2.0f / -currentViewport.height;
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

		glBindBuffer(GL_ARRAY_BUFFER, (uintptr_t)batch.vertexBuffer->getHandle());
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

		attrLoc = glGetAttribLocation(program, "inTEXINDEX");
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

}