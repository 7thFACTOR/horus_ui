#pragma once
#include "horus.h"
#include <string>
#include <vector>
#include <d3d12.h>
#include <dxgi1_4.h>

namespace hui
{
struct Dx12Texture
{
	Dx12Texture() {}
	Dx12Texture(u32 newWidth, u32 newHeight, Rgba32* pixels);
	Dx12Texture(u32 newWidth, u32 newHeight);
	~Dx12Texture();
	void destroy();
	void resize(u32 newWidth, u32 newHeight);
	void updateData(Rgba32* pixels);
	void updateRectData(const Rect& rect, Rgba32* pixels);
	HTexture getHandle() const { return (HTexture)(void*)this; }
	u32 getWidth() const { return width; }
	u32 getHeight() const { return height; }

	ID3D12Resource* handle = nullptr;
	ID3D12Resource* uploadBuffer = nullptr;
	u32 srvHeapIndex = 0;
	u32 width = 0;
	u32 height = 0;
	bool isUploaded = false; // tracks state: false = COPY_DEST, true = PIXEL_SHADER_RESOURCE
};

struct Dx12VertexBuffer
{
	Dx12VertexBuffer();
	Dx12VertexBuffer(u32 count, Vertex* vertices);
	~Dx12VertexBuffer();
	void resize(u32 count);
	void updateData(Vertex* vertices, u32 startVertexIndex, u32 count);
	void destroy();
	void create(u32 count);

	ID3D12Resource* handle = nullptr;
	D3D12_VERTEX_BUFFER_VIEW view{};
	u32 count = 0;
};

void createWindowDx12(HWND hwnd, struct SdlWindowProxy* proxy, const Rect& rect);
void setCurrentWindowDx12(struct SdlWindowProxy* proxy);
void resizeSwapchainForSdlWindowDx12(struct SdlWindowProxy* proxy);
bool initDx12(Services& services);
void shutdownDx12(Services& services);

}
