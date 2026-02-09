#pragma once
#include "horus.h"
#include <glad/gl.h>

namespace hui
{
struct OpenGLTexture
{
	OpenGLTexture() {}
	OpenGLTexture(u32 count, u32 newWidth, u32 newHeight, Rgba32* pixels);
	OpenGLTexture(u32 count, u32 newWidth, u32 newHeight);
	~OpenGLTexture();
	void destroy();

	void resize(u32 count, u32 newWidth, u32 newHeight) override;
	void updateData(Rgba32* pixels) override;
	void updateRectData(u32 textureIndex, const Rect& rect, Rgba32* pixels) override;
	HGraphicsApiTexture getHandle() const override { return (HGraphicsApiTexture)handle; }
	virtual u32 getWidth() const override { return width; }
	virtual u32 getHeight() const override { return height; }

	GLuint handle = 0;
	u32 width = 0;
	u32 height = 0;
};

}