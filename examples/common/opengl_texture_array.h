#pragma once
#include "horus.h"
#include "horus_interfaces.h"
#define GLEW_STATIC
#include <GL/glew.h>

namespace hui
{
struct OpenGLTextureArray : TextureArray
{
    OpenGLTextureArray() {}
    OpenGLTextureArray(u32 count, u32 newWidth, u32 newHeight, Rgba32* pixels);
    OpenGLTextureArray(u32 count, u32 newWidth, u32 newHeight);
    ~OpenGLTextureArray();
    void destroy();

    void resize(u32 count, u32 newWidth, u32 newHeight) override;
    void updateData(Rgba32* pixels) override;
    void updateLayerData(u32 textureIndex, Rgba32* pixels) override;
    void updateRectData(u32 textureIndex, const Rect& rect, Rgba32* pixels) override;
    GraphicsApiTexture getHandle() const override { return (GraphicsApiTexture)handle; }
    virtual u32 getWidth() const override { return width; }
    virtual u32 getHeight() const override { return height; }
    virtual u32 getCount() const override { return textureCount; }

    GLuint handle = 0;
    u32 width = 0;
    u32 height = 0;
    u32 textureCount = 0;
};

}