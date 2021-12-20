#pragma once
#include "horus_interfaces.h"
#define GLEW_STATIC
#include <GL/glew.h>

namespace hui
{
struct OpenGLVertexBuffer : VertexBuffer
{
	OpenGLVertexBuffer();
	OpenGLVertexBuffer(u32 count, Vertex* vertices);
	virtual ~OpenGLVertexBuffer();
	virtual void resize(u32 count) override;
	virtual void updateData(Vertex* vertices, u32 startVertexIndex, u32 count) override;
	virtual void destroy();
	virtual HGraphicsApiVertexBuffer getHandle() const override;
	void create(u32 count);

	GLuint vbHandle = 0;
};

}