#include "opengl_texture_array.h"
#include "opengl_graphics_provider.h"

namespace hui
{
OpenGLTexture::OpenGLTexture(u32 count, u32 newWidth, u32 newHeight, Rgba32* pixels)
{
	resize(count, newWidth, newHeight);
	updateData(pixels);
}

OpenGLTexture::OpenGLTexture(u32 count, u32 newWidth, u32 newHeight)
{
	resize(count, newWidth, newHeight);
}

OpenGLTexture::~OpenGLTexture()
{
	destroy();
}

void OpenGLTexture::resize(u32 count, u32 newWidth, u32 newHeight)
{
	textureCount = count;

	if (!handle)
	{
		glGenTextures(1, &handle);
	}

	width = newWidth;
	height = newHeight;
	glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
	OGL_CHECK_ERROR;
	glTexImage3D(GL_TEXTURE_2D_ARRAY,
		0,
		GL_RGBA8,
		width, height, textureCount, // width,height,depth
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		0);
	OGL_CHECK_ERROR;
}

void OpenGLTexture::updateData(Rgba32* pixels)
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
	OGL_CHECK_ERROR;
	glTexSubImage3D(
		GL_TEXTURE_2D_ARRAY,
		0, 0, 0, 0,
		width, height, 1,
		GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	OGL_CHECK_ERROR;
}

void OpenGLTexture::updateLayerData(u32 textureIndex, Rgba32* pixels)
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
	OGL_CHECK_ERROR;
	glTexSubImage3D(
		GL_TEXTURE_2D_ARRAY,
		0, //mip
		0, //x
		0, //y
		textureIndex,
		width, height, 1,
		GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	OGL_CHECK_ERROR;
}

void OpenGLTexture::updateRectData(u32 textureIndex, const Rect& rect, Rgba32* pixels)
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
	OGL_CHECK_ERROR;
	glTexSubImage3D(
		GL_TEXTURE_2D_ARRAY,
		0, rect.x, rect.y, textureIndex, rect.width, rect.height,
		0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
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

}