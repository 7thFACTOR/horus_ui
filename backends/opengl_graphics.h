#pragma once
#include "horus.h"
#include "horus_interfaces.h"
#include <string>

#define GLEW_STATIC
#include <GL/glew.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace hui
{
#ifndef _DEBUG
#define OGL_CHECK_ERROR { char errStr[1024] = {0}; sprintf(errStr, "File: %s, line: %d", __FILE__, __LINE__); checkErrorGL(errStr); };
#else
#define OGL_CHECK_ERROR
#endif

extern void checkErrorGL(const char* where);

struct OpenGLRenderTarget
{
	u32 width = 0;
	u32 height = 0;
	GLuint frameBuffer = 0;
	GLuint texture = 0;
};

struct OpenGLGraphicsProvider : GraphicsProvider
{
	OpenGLGraphicsProvider();
	~OpenGLGraphicsProvider();
	bool initialize() override;
	void shutdown() override;
	ApiType getApiType() const { return GraphicsProvider::ApiType::OpenGL; }
	TextureArray* createTextureArray() override;
	VertexBuffer* createVertexBuffer() override;
	GraphicsApiRenderTarget createRenderTarget(u32 width, u32 height) override;
	void destroyRenderTarget(GraphicsApiRenderTarget rt) override;
	void setRenderTarget(GraphicsApiRenderTarget rt) override;
	void commitRenderState();
	void setViewport(const Point& windowSize, const Rect& viewport) override;
	void clear(const Color& color) override;
	void draw(struct RenderBatch* batches, u32 count) override;

	Rect currentViewport;
	GLuint vertexShader = 0;
	GLuint pixelShader = 0;
	GLuint program = 0;
};
}