#include "dx11_graphics.h"
#include <iostream>
#include <stdexcept>
#include <string.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace hui
{
#ifndef _WINDOWS
#define _WINDOWS 1
#endif
extern ID3D11Device* g_dx11Device;
extern ID3D11DeviceContext* g_dx11DeviceContext;

#define device g_dx11Device
#define deviceContext g_dx11DeviceContext

static Rect currentViewport;

static ID3D11VertexShader* vertexShader = nullptr;
static ID3D11PixelShader* pixelShader = nullptr;
static ID3D11InputLayout* inputLayout = nullptr;
static ID3D11Buffer* constantBuffer = nullptr;
static ID3D11SamplerState* samplerState = nullptr;
static ID3D11BlendState* blendState = nullptr;
static ID3D11RasterizerState* rasterizerState = nullptr;
static ID3D11DepthStencilState* depthStencilState = nullptr;

struct Dx11SharedVertexBuffer
{
    ID3D11Buffer* handle = nullptr;
    u32 count = 0;
    
    void resize(u32 newCount) {
        if (!device) return;
        if (handle) { handle->Release(); handle = nullptr; }
        count = newCount;
        D3D11_BUFFER_DESC desc{};
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = sizeof(Vertex) * count;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        device->CreateBuffer(&desc, nullptr, &handle);
    }
};
static Dx11SharedVertexBuffer sharedVb;

static const char* uiShaderSource = R"(
cbuffer constants : register(b0) {
    matrix mvp;
};
Texture2D tex : register(t0);
SamplerState smp : register(s0);

struct VS_INPUT {
    float2 pos : POSITION;
    float2 uv  : TEXCOORD0;
    float4 color : COLOR0;
};
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
    float4 color : COLOR0;
};

PS_INPUT VSMain(VS_INPUT input) {
    PS_INPUT output;
    output.pos = mul(mvp, float4(input.pos, 0.0f, 1.0f));
    output.uv  = input.uv;
    output.color = input.color;
    return output;
}

float4 PSMain(PS_INPUT input) : SV_TARGET {
    return input.color * tex.Sample(smp, input.uv);
}
)";

// -------------------------------------------------------------------------
// Dx11Texture implementation
// -------------------------------------------------------------------------

Dx11Texture::Dx11Texture(u32 newWidth, u32 newHeight, Rgba32* pixels)
{
	resize(newWidth, newHeight);
	updateData(pixels);
}

Dx11Texture::Dx11Texture(u32 newWidth, u32 newHeight)
{
	resize(newWidth, newHeight);
}

Dx11Texture::~Dx11Texture()
{
	destroy();
}

void Dx11Texture::resize(u32 newWidth, u32 newHeight)
{
	if (!device) return;

	destroy();

	width = newWidth;
	height = newHeight;

	// create texture
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	if (FAILED(device->CreateTexture2D(&desc, nullptr, &handle)))
	{
		printf("Dx11Texture::resize failed to create texture\n");
		return;
	}

	// create shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	if (FAILED(device->CreateShaderResourceView(handle, &srvDesc, &view)))
	{
		printf("Dx11Texture::resize failed to create shader resource view\n");
	}
}

void Dx11Texture::updateData(Rgba32* pixels)
{
	if (!deviceContext || !handle || !pixels) return;

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	if (SUCCEEDED(deviceContext->Map(handle, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
	{
		u8* dest = (u8*)mappedResource.pData;
		u8* src = (u8*)pixels;
		for (u32 y = 0; y < height; ++y)
		{
			memcpy(dest, src, width * sizeof(Rgba32));
			dest += mappedResource.RowPitch;
			src += width * sizeof(Rgba32);
		}
		deviceContext->Unmap(handle, 0);
	}
}

void Dx11Texture::updateRectData(const Rect& rect, Rgba32* pixels)
{
	// updating a rect normally requires a staging texture or map write discard limitation handling
	// in dx11, D3D11_MAP_WRITE_NO_OVERWRITE or using updatesubresource is better for regions.
	// for dynamic textures, map discard replaces everything.
	if (!deviceContext || !handle || !pixels) return;
	printf("Dx11Texture::updateRectData not fully implemented for dynamic rect update\n");
}

void Dx11Texture::destroy()
{
	if (view)
	{
		view->Release();
		view = nullptr;
	}
	if (handle)
	{
		handle->Release();
		handle = nullptr;
	}
}

// -------------------------------------------------------------------------
// Dx11VertexBuffer implementation
// -------------------------------------------------------------------------

Dx11VertexBuffer::Dx11VertexBuffer() {}

Dx11VertexBuffer::Dx11VertexBuffer(u32 count, Vertex* vertices)
{
	create(count);
	updateData(vertices, 0, count);
}

Dx11VertexBuffer::~Dx11VertexBuffer()
{
	destroy();
}

void Dx11VertexBuffer::create(u32 count)
{
	destroy();
	resize(count);
}

void Dx11VertexBuffer::resize(u32 count)
{
	if (!device) return;
	if (count == 0) return;

	destroy();
	this->count = count;

	D3D11_BUFFER_DESC desc{};
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = sizeof(Vertex) * count;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	if (FAILED(device->CreateBuffer(&desc, nullptr, &handle)))
	{
		printf("Dx11VertexBuffer::resize failed to create vertex buffer\n");
	}
}

void Dx11VertexBuffer::updateData(Vertex* vertices, u32 startVertexIndex, u32 count)
{
	if (!deviceContext || !handle || !vertices || count == 0) return;

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	if (SUCCEEDED(deviceContext->Map(handle, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
	{
		Vertex* dest = (Vertex*)mappedResource.pData;
		memcpy(dest + startVertexIndex, vertices + startVertexIndex, sizeof(Vertex) * count);
		deviceContext->Unmap(handle, 0);
	}
}

void Dx11VertexBuffer::destroy()
{
	if (handle)
	{
		handle->Release();
		handle = nullptr;
	}
	count = 0;
}

// -------------------------------------------------------------------------
// ui rendering services callbacks
// -------------------------------------------------------------------------

static void setViewport(const Point& windowSize, const Rect& viewport)
{
	currentViewport = viewport;
	if (deviceContext)
	{
		D3D11_VIEWPORT vp{};
		vp.TopLeftX = viewport.x;
		vp.TopLeftY = viewport.y;
		vp.Width = viewport.width;
		vp.Height = viewport.height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		deviceContext->RSSetViewports(1, &vp);
	}
}

static void clearBackbuffer(const Color& color)
{
	if (!deviceContext) return;
	
	ID3D11RenderTargetView* rtv = nullptr;
	deviceContext->OMGetRenderTargets(1, &rtv, nullptr);
	if (rtv)
	{
		float clearColor[4] = { color.r, color.g, color.b, color.a };
		deviceContext->ClearRenderTargetView(rtv, clearColor);
		rtv->Release();
	}
}

static void draw(Vertex* vertices, u32 vertexCount, struct RenderBatch* batches, u32 count)
{
	if (!deviceContext || vertexCount == 0) return;

	// 1. update the vertexbuffer with `vertices` data
	if (sharedVb.count < vertexCount) sharedVb.resize(vertexCount);

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	if (SUCCEEDED(deviceContext->Map(sharedVb.handle, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
	{
		memcpy(mappedResource.pData, vertices, sizeof(Vertex) * vertexCount);
		deviceContext->Unmap(sharedVb.handle, 0);
	}

	// update mvp matrix
	f32 m[4][4] = { 0 };
	m[0][0] = 2.0f / currentViewport.width;
	m[1][1] = -2.0f / currentViewport.height;
	m[2][2] = 1.0f;
	m[3][0] = -1.0f;
	m[3][1] = 1.0f;
	m[3][3] = 1.0f;

	if (SUCCEEDED(deviceContext->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
	{
		memcpy(mappedResource.pData, m, sizeof(m));
		deviceContext->Unmap(constantBuffer, 0);
	}

	deviceContext->IASetInputLayout(inputLayout);
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &sharedVb.handle, &stride, &offset);

	deviceContext->VSSetShader(vertexShader, nullptr, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);
	deviceContext->PSSetShader(pixelShader, nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, &samplerState);

	deviceContext->OMSetBlendState(blendState, nullptr, 0xffffffff);
	deviceContext->OMSetDepthStencilState(depthStencilState, 0);
	deviceContext->RSSetState(rasterizerState);

	for (u32 i = 0; i < count; i++)
	{
		auto& batch = batches[i];
		D3D11_PRIMITIVE_TOPOLOGY top = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		if (batch.primitiveType == RenderBatch::PrimitiveType::TriangleStrip) top = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

		deviceContext->IASetPrimitiveTopology(top);

		auto srv = (ID3D11ShaderResourceView*)batch.texture;
		if (srv)
		{
			deviceContext->PSSetShaderResources(0, 1, &srv);
		}

		deviceContext->Draw(batch.vertexCount, batch.startVertexIndex);
	}
}

// -------------------------------------------------------------------------
// initialization & shutdown
// -------------------------------------------------------------------------

bool initDx11(Services& services)
{
	printf("Initializing HorusUI Direct3D 11 provider...\n");

	if (!device)
	{
		printf("Direct3D 11 device not created by window manager!\n");
		return false;
	}

	// compile shaders
	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* errBlob = nullptr;
	D3DCompile(uiShaderSource, strlen(uiShaderSource), nullptr, nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, &errBlob);
	if (errBlob) { printf("DX11 VS Error: %s\n", (char*)errBlob->GetBufferPointer()); errBlob->Release(); }
	if (vsBlob) device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);

	ID3DBlob* psBlob = nullptr;
	D3DCompile(uiShaderSource, strlen(uiShaderSource), nullptr, nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, &errBlob);
	if (errBlob) { printf("DX11 PS Error: %s\n", (char*)errBlob->GetBufferPointer()); errBlob->Release(); }
	if (psBlob) device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);

	D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
	if (vsBlob) device->CreateInputLayout(layout, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);

    if (vsBlob) vsBlob->Release();
    if (psBlob) psBlob->Release();

    // Constant buffer
    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.ByteWidth = sizeof(f32) * 16;
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&cbDesc, nullptr, &constantBuffer);

    // Sampler state
    D3D11_SAMPLER_DESC sampDesc{};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    device->CreateSamplerState(&sampDesc, &samplerState);

    // Blend state
    D3D11_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState(&blendDesc, &blendState);

    // Rasterizer state
    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.ScissorEnable = FALSE;
    rsDesc.DepthClipEnable = FALSE;
    device->CreateRasterizerState(&rsDesc, &rasterizerState);

    // Depth Stencil State
    D3D11_DEPTH_STENCIL_DESC dsDesc{};
    dsDesc.DepthEnable = FALSE;
    dsDesc.StencilEnable = FALSE;
    device->CreateDepthStencilState(&dsDesc, &depthStencilState);

	// hook into Horus UI Services
	services.setViewport = setViewport;
	services.clearBackbuffer = clearBackbuffer;
	services.draw = draw;
	services.getGfxApiName = []() -> const char* { return "Direct3D 11"; };

	printf("Direct3D 11 backend initialized successfully.\n");
	return true;
}

void shutdownDx11(Services& services)
{
	if (sharedVb.handle) { sharedVb.handle->Release(); sharedVb.handle = nullptr; }
	if (vertexShader) { vertexShader->Release(); vertexShader = nullptr; }
	if (pixelShader) { pixelShader->Release(); pixelShader = nullptr; }
	if (inputLayout) { inputLayout->Release(); inputLayout = nullptr; }
	if (constantBuffer) { constantBuffer->Release(); constantBuffer = nullptr; }
	if (samplerState) { samplerState->Release(); samplerState = nullptr; }
	if (blendState) { blendState->Release(); blendState = nullptr; }
	if (rasterizerState) { rasterizerState->Release(); rasterizerState = nullptr; }
	if (depthStencilState) { depthStencilState->Release(); depthStencilState = nullptr; }

	if (deviceContext)
	{
		deviceContext->ClearState();
		deviceContext->Release();
		deviceContext = nullptr;
	}

	if (device)
	{
		device->Release();
		device = nullptr;
	}

	services.setViewport = nullptr;
	services.clearBackbuffer = nullptr;
	services.draw = nullptr;
	services.getGfxApiName = nullptr;
}

}
