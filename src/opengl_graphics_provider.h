#pragma once
#include "horus.h"
#include "types.h"
#include "horus_interfaces.h"
#include <string>

#define GLEW_STATIC
#include <GL/glew.h>

#ifdef _WINDOWS
#include <windows.h>
#endif

namespace hui
{
#define HUI_USE_RENDER_STATISTICS

#ifndef HUI_GL_DEBUG
#define OGL_CHECK_ERROR { char sss[1024] = {0}; sprintf(sss, "File: %s, line: %d", __FILE__, __LINE__); checkErrorGL(sss); };
#else
#define OGL_CHECK_ERROR
#endif
	
extern void checkErrorGL(const std::string& where);

struct OpenGLRenderTarget
{
	u32 width;
	u32 height;
	GLuint frameBuffer;
	GLuint texture;
};

struct OpenGLGraphicsProvider : GraphicsProvider
{
	OpenGLGraphicsProvider();
	~OpenGLGraphicsProvider();

	TextureArray* createTextureArray() override;
	void deleteTextureArray(TextureArray* texture) override;
	VertexBuffer* createVertexBuffer() override;
	void deleteVertexBuffer(VertexBuffer* vb) override;
	GraphicsApiRenderTarget createRenderTarget(u32 width, u32 height) override;
	void destroyRenderTarget(GraphicsApiRenderTarget rt) override;
	void setRenderTarget(GraphicsApiRenderTarget rt) override;
	void commitRenderState() override;
	void setViewport(const Point& windowSize, const Rect& viewport) override;
	void clear(const Color& color) override;
	void draw(struct RenderBatch* batches, u32 count) override;

	Rect currentViewport;
	GLuint vertexShader = 0;
	GLuint pixelShader = 0;
	GLuint program = 0;
};
}