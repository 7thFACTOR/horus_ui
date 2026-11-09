#pragma once
#include "horus.h"
#include <string>
#include <glad/gl.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace hui
{
struct OpenGLTexture
{
	OpenGLTexture() {}
	OpenGLTexture(u32 newWidth, u32 newHeight, Rgba32* pixels);
	OpenGLTexture(u32 newWidth, u32 newHeight);
	~OpenGLTexture();
	void destroy();
	void resize(u32 newWidth, u32 newHeight);
	void updateData(Rgba32* pixels);
	void updateRectData(const Rect& rect, Rgba32* pixels);
	HTexture getHandle() const { return (HTexture)handle; }
	u32 getWidth() const { return width; }
	u32 getHeight() const { return height; }

	GLuint handle = 0;
	u32 width = 0;
	u32 height = 0;
};

struct OpenGLVertexBuffer
{
	OpenGLVertexBuffer();
	OpenGLVertexBuffer(u32 count, Vertex* vertices);
	~OpenGLVertexBuffer();
	void resize(u32 count);
	void updateData(Vertex* vertices, u32 startVertexIndex, u32 count);
	void destroy();
	void create(u32 count);

	GLuint handle = 0;
	u32 count = 0;
};

bool initOpenGL(Services& services);
void shutdownOpenGL(Services& services);
}