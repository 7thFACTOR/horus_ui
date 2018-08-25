#include "opengl_vertex_buffer.h"
#include "opengl_graphics_provider.h"

#define GLEW_STATIC
#include <GL/glew.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include <string.h>

namespace hui
{
OpenGLVertexBuffer::OpenGLVertexBuffer()
{
	create(1);
}

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
	glGenBuffers(1, (GLuint*)&vbHandle);
	OGL_CHECK_ERROR;
	resize(count);
}

void OpenGLVertexBuffer::resize(u32 count)
{
	glBindBuffer(GL_ARRAY_BUFFER, (GLuint)vbHandle);
	OGL_CHECK_ERROR;
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(Vertex) * count,
		nullptr,
		GL_DYNAMIC_DRAW);
	OGL_CHECK_ERROR;
}

void OpenGLVertexBuffer::updateData(Vertex* vertices, u32 startVertexIndex, u32 count)
{
	glBindBuffer(GL_ARRAY_BUFFER, vbHandle);
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
	glDeleteBuffers(1, &vbHandle);
	vbHandle = 0;
}

GraphicsApiVertexBuffer OpenGLVertexBuffer::getHandle() const
{
	return (GraphicsApiVertexBuffer)vbHandle;
}

}
