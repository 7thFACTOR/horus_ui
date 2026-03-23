#pragma once
#include "horus.h"
#include <string>
#include <vector>
#include <d3d11.h>
#include <dxgi.h>

namespace hui
{
struct Dx11Texture
{
	Dx11Texture() {}
	Dx11Texture(u32 newWidth, u32 newHeight, Rgba32* pixels);
	Dx11Texture(u32 newWidth, u32 newHeight);
	~Dx11Texture();
	void destroy();
	void resize(u32 newWidth, u32 newHeight);
	void updateData(Rgba32* pixels);
	void updateRectData(const Rect& rect, Rgba32* pixels);
	HTexture getHandle() const { return (HTexture)view; }
	u32 getWidth() const { return width; }
	u32 getHeight() const { return height; }

	ID3D11Texture2D* handle = nullptr;
	ID3D11ShaderResourceView* view = nullptr;
	u32 width = 0;
	u32 height = 0;
};

struct Dx11VertexBuffer
{
	Dx11VertexBuffer();
	Dx11VertexBuffer(u32 count, Vertex* vertices);
	~Dx11VertexBuffer();
	void resize(u32 count);
	void updateData(Vertex* vertices, u32 startVertexIndex, u32 count);
	void destroy();
	void create(u32 count);

	ID3D11Buffer* handle = nullptr;
	u32 count = 0;
};

// initializes the directx 11 graphics backend
bool initDx11(Services& services);

// shuts down the directx 11 graphics backend
void shutdownDx11(Services& services);

}
