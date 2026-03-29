#include "dx12_graphics.h"
#include <d3dcompiler.h>
#include <iostream>
#include <stdexcept>
#include <string.h>
#include <unordered_set>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace hui
{
#ifndef _WINDOWS
#define _WINDOWS 1
#endif
extern ID3D12Device* g_dx12Device;
extern ID3D12CommandQueue* g_dx12CommandQueue;
extern ID3D12CommandAllocator* g_dx12CommandAllocator;
extern ID3D12GraphicsCommandList* g_dx12CommandList;

#define device g_dx12Device
#define commandQueue g_dx12CommandQueue
#define commandAllocator g_dx12CommandAllocator
#define commandList g_dx12CommandList

static Rect currentViewport;
static ID3D12RootSignature* rootSignature = nullptr;
static ID3D12PipelineState* pipelineState = nullptr;
static D3D12_CPU_DESCRIPTOR_HANDLE currentRtvHandle = {};

#define NUM_FRAMES_IN_FLIGHT 3

struct FrameData {
    ID3D12CommandAllocator* commandAllocator = nullptr;
    UINT64 fenceValue = 0;
    std::vector<Dx12VertexBuffer*> vertexBuffers;
    u32 currentVbIndex = 0;

    Dx12VertexBuffer* getNextVertexBuffer() {
        if (currentVbIndex >= vertexBuffers.size()) {
            vertexBuffers.push_back(new Dx12VertexBuffer());
        }
        return vertexBuffers[currentVbIndex++];
    }

    void destroy() {
        if (commandAllocator) { commandAllocator->Release(); commandAllocator = nullptr; }
        for (auto vb : vertexBuffers) { delete vb; }
        vertexBuffers.clear();
    }
};

static FrameData g_frames[NUM_FRAMES_IN_FLIGHT];
static u32 g_currentFrameIndex = 0;

static ID3D12DescriptorHeap* srvHeap = nullptr;
static u32 srvHeapIndexCount = 1;
static ID3D12CommandAllocator* uploadAllocator = nullptr;
static ID3D12GraphicsCommandList* uploadCmdList = nullptr;
static bool commandListRecording = false;
static std::unordered_set<ID3D12Resource*> rtStateResources;

static ID3D12Fence* globalFence = nullptr;
static UINT64 fenceValue = 0;
static HANDLE fenceEvent = nullptr;
static UINT64 pendingUploadFenceValue = 0; // tracks last submitted (but not waited) upload

// -------------------------------------------------------------------------
// Dx12Texture implementation
// -------------------------------------------------------------------------

Dx12Texture::Dx12Texture(u32 newWidth, u32 newHeight, Rgba32* pixels)
{
	resize(newWidth, newHeight);
	updateData(pixels);
}

Dx12Texture::Dx12Texture(u32 newWidth, u32 newHeight)
{
	resize(newWidth, newHeight);
}

Dx12Texture::~Dx12Texture()
{
	destroy();
}

void Dx12Texture::resize(u32 newWidth, u32 newHeight)
{
	if (!device) return;

	destroy();

	width = newWidth;
	height = newHeight;

	// create texture resource
	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = width;
	desc.Height = height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	if (FAILED(device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&handle))))
	{
		printf("Dx12Texture::resize failed to create texture resource\n");
		return;
	}

	UINT rowPitch = width * 4;
	UINT alignedPitch = (rowPitch + 255) & ~255;
	UINT64 uploadBufferSize = alignedPitch * height;
	
	D3D12_HEAP_PROPERTIES uploadHeapProps{};
	uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC uploadDesc{};
	uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	uploadDesc.Alignment = 0;
	uploadDesc.Width = uploadBufferSize;
	uploadDesc.Height = 1;
	uploadDesc.DepthOrArraySize = 1;
	uploadDesc.MipLevels = 1;
	uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
	uploadDesc.SampleDesc.Count = 1;
	uploadDesc.SampleDesc.Quality = 0;
	uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	if (FAILED(device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&uploadDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuffer))))
	{
		printf("Dx12Texture::resize failed to create upload buffer\n");
	}

	this->srvHeapIndex = srvHeapIndexCount++;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvViewDesc{};
	srvViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvViewDesc.Texture2D.MipLevels = 1;
	
	D3D12_CPU_DESCRIPTOR_HANDLE handleCpu = srvHeap->GetCPUDescriptorHandleForHeapStart();
	handleCpu.ptr += this->srvHeapIndex * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	device->CreateShaderResourceView(handle, &srvViewDesc, handleCpu);
}

void Dx12Texture::updateData(Rgba32* pixels)
{
	if (!device || !handle || !uploadBuffer || !pixels) return;

	// Ensure the fence + event exist (created lazily)
	if (!globalFence) {
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&globalFence));
		fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	// Wait for the PREVIOUS upload to finish before resetting the upload allocator.
	// We do NOT wait for the current upload — it is submitted asynchronously and
	// will complete on the GPU before any draw commands issued after this point
	// (queue ordering guarantees this).
	if (pendingUploadFenceValue > 0 && globalFence->GetCompletedValue() < pendingUploadFenceValue) {
		globalFence->SetEventOnCompletion(pendingUploadFenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	UINT rowPitch = width * sizeof(Rgba32);
	UINT alignedPitch = (rowPitch + 255) & ~255;

	void* mappedData = nullptr;
	D3D12_RANGE readRange{0, 0};
	if (SUCCEEDED(uploadBuffer->Map(0, &readRange, &mappedData)))
	{
		u8* dst = (u8*)mappedData;
		u8* src = (u8*)pixels;
		for (u32 y = 0; y < height; ++y) {
			memcpy(dst, src, rowPitch);
			dst += alignedPitch;
			src += rowPitch;
		}
		uploadBuffer->Unmap(0, nullptr);
	}

	uploadAllocator->Reset();
	uploadCmdList->Reset(uploadAllocator, nullptr);

	// If already uploaded the texture is in PIXEL_SHADER_RESOURCE state;
	// transition it back to COPY_DEST before copying.
	if (isUploaded) {
		D3D12_RESOURCE_BARRIER toCopyDest{};
		toCopyDest.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		toCopyDest.Transition.pResource = handle;
		toCopyDest.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		toCopyDest.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		toCopyDest.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		uploadCmdList->ResourceBarrier(1, &toCopyDest);
	}

	D3D12_TEXTURE_COPY_LOCATION dst{};
	dst.pResource = handle;
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION src{};
	src.pResource = uploadBuffer;
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

	D3D12_RESOURCE_DESC desc = handle->GetDesc();
	device->GetCopyableFootprints(&desc, 0, 1, 0, &src.PlacedFootprint, nullptr, nullptr, nullptr);
	uploadCmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

	// Transition to PIXEL_SHADER_RESOURCE so it is ready for drawing.
	D3D12_RESOURCE_BARRIER toSRV{};
	toSRV.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	toSRV.Transition.pResource = handle;
	toSRV.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	toSRV.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	toSRV.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	uploadCmdList->ResourceBarrier(1, &toSRV);
	uploadCmdList->Close();

	ID3D12CommandList* ppCmds[] = { uploadCmdList };
	commandQueue->ExecuteCommandLists(1, ppCmds);

	// Signal the fence async — do NOT wait here. Any draw commands submitted
	// after this on the same queue are guaranteed to execute after this upload.
	fenceValue++;
	pendingUploadFenceValue = fenceValue;
	commandQueue->Signal(globalFence, fenceValue);

	isUploaded = true;
}

void Dx12Texture::updateRectData(const Rect& rect, Rgba32* pixels)
{
	if (!device || !handle || !pixels) return;
	// similar to updateData but updating a specific region copy footprint
}

void Dx12Texture::destroy()
{
	if (uploadBuffer)
	{
		uploadBuffer->Release();
		uploadBuffer = nullptr;
	}
	if (handle)
	{
		handle->Release();
		handle = nullptr;
	}
}

// -------------------------------------------------------------------------
// Dx12VertexBuffer implementation
// -------------------------------------------------------------------------

Dx12VertexBuffer::Dx12VertexBuffer() {}

Dx12VertexBuffer::Dx12VertexBuffer(u32 count, Vertex* vertices)
{
	create(count);
	updateData(vertices, 0, count);
}

Dx12VertexBuffer::~Dx12VertexBuffer()
{
	destroy();
}

void Dx12VertexBuffer::create(u32 count)
{
	destroy();
	resize(count);
}

void Dx12VertexBuffer::resize(u32 count)
{
	if (!device) return;
	if (count == 0) return;

	destroy();
	this->count = count;

	UINT bufferSize = sizeof(Vertex) * count;

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD; // using upload heap for frequent updates

	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = bufferSize;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	if (FAILED(device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&handle))))
	{
		printf("Dx12VertexBuffer::resize failed to create vertex buffer\n");
		return;
	}

	view.BufferLocation = handle->GetGPUVirtualAddress();
	view.StrideInBytes = sizeof(Vertex);
	view.SizeInBytes = bufferSize;
}

void Dx12VertexBuffer::updateData(Vertex* vertices, u32 startVertexIndex, u32 count)
{
	if (!device || !handle || !vertices || count == 0) return;

	void* mappedData = nullptr;
	D3D12_RANGE readRange{0, 0};
	if (SUCCEEDED(handle->Map(0, &readRange, &mappedData)))
	{
		Vertex* dest = (Vertex*)mappedData;
		memcpy(dest + startVertexIndex, vertices + startVertexIndex, sizeof(Vertex) * count);
		handle->Unmap(0, nullptr);
	}
}

void Dx12VertexBuffer::destroy()
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
	if (commandList)
	{
		D3D12_VIEWPORT vp{};
		vp.TopLeftX = viewport.x;
		vp.TopLeftY = viewport.y;
		vp.Width = viewport.width;
		vp.Height = viewport.height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		commandList->RSSetViewports(1, &vp);

        D3D12_RECT scissor{};
        scissor.left = (LONG)viewport.x;
        scissor.top = (LONG)viewport.y;
        scissor.right = (LONG)(viewport.x + viewport.width);
        scissor.bottom = (LONG)(viewport.y + viewport.height);
        commandList->RSSetScissorRects(1, &scissor);
	}
}

static void clearBackbuffer(const Color& color)
{
	if (!commandList) return;
	float clearColor[4] = { color.r, color.g, color.b, color.a };
	commandList->ClearRenderTargetView(currentRtvHandle, clearColor, 0, nullptr);
}

static void draw(Vertex* vertices, u32 vertexCount, struct RenderBatch* batches, u32 count)
{
	if (!commandList || !commandListRecording) return;
	if (!vertices || vertexCount == 0 || !batches || count == 0) return;

	// 1. set root signature and pipeline state
    commandList->SetGraphicsRootSignature(rootSignature);
    commandList->SetPipelineState(pipelineState);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 2. update shared vertex buffer
	auto vb = g_frames[g_currentFrameIndex].getNextVertexBuffer();
	if (vb->count < vertexCount) vb->resize(vertexCount);
	if (!vb->handle) return;
	vb->updateData(vertices, 0, vertexCount);
	commandList->IASetVertexBuffers(0, 1, &vb->view);

	// 3. update mvp
	f32 m[16] = { 0 };
	m[0] = 2.0f / currentViewport.width;
	m[5] = -2.0f / currentViewport.height;
	m[10] = 1.0f;
	m[12] = -1.0f;
	m[13] = 1.0f;
	m[15] = 1.0f;
    commandList->SetGraphicsRoot32BitConstants(0, 16, m, 0);

	ID3D12DescriptorHeap* heaps[] = { srvHeap };
	commandList->SetDescriptorHeaps(1, heaps);

	// 4. iterate batches and draw
    for (u32 i = 0; i < count; i++) {
        auto& batch = batches[i];
        if (batch.vertexCount == 0) continue;
        
        u32 texIndex = 0;
        if (batch.texture) {
            auto tex = (Dx12Texture*)batch.texture;
            texIndex = tex->srvHeapIndex;
        }
        
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
        gpuHandle.ptr += texIndex * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        commandList->SetGraphicsRootDescriptorTable(1, gpuHandle);
        
        commandList->DrawInstanced(batch.vertexCount, 1, batch.startVertexIndex, 0);
    }
}

static void flushDX12Queue() {
    if (!device || !commandQueue) return;

    if (!globalFence) {
        device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&globalFence));
        fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    }
    
    fenceValue++;
    commandQueue->Signal(globalFence, fenceValue);
    
    if (globalFence->GetCompletedValue() < fenceValue) {
        globalFence->SetEventOnCompletion(fenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }
}

static void submitAndAdvance() {
    if (!commandListRecording) return;
    commandList->Close();
    ID3D12CommandList* ppCommandLists[] = { commandList };
    commandQueue->ExecuteCommandLists(1, ppCommandLists);
    
    if (!globalFence) {
        device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&globalFence));
        fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    }

    fenceValue++;
    g_frames[g_currentFrameIndex].fenceValue = fenceValue;
    commandQueue->Signal(globalFence, fenceValue);
    
    g_currentFrameIndex = (g_currentFrameIndex + 1) % NUM_FRAMES_IN_FLIGHT;
    commandListRecording = false;
}

static void beginRecordingIfNeeded() {
    if (commandListRecording) return;
    
    if (!globalFence) {
        device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&globalFence));
        fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    }

    if (globalFence->GetCompletedValue() < g_frames[g_currentFrameIndex].fenceValue) {
        globalFence->SetEventOnCompletion(g_frames[g_currentFrameIndex].fenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }
    
    g_frames[g_currentFrameIndex].commandAllocator->Reset();
    commandList->Reset(g_frames[g_currentFrameIndex].commandAllocator, pipelineState);
    g_frames[g_currentFrameIndex].currentVbIndex = 0;
    commandListRecording = true;
}

void dx12PreResize(ID3D12Resource** buffers, int count)
{
    if (commandListRecording) {
        submitAndAdvance();
    }
    flushDX12Queue();
    
    for (int i = 0; i < count; i++) {
        if (buffers[i]) {
            rtStateResources.erase(buffers[i]);
        }
    }
}

void dx12SetCurrentRenderTarget(SIZE_T rtvPtr, ID3D12Resource* backBuffer)
{
    if (commandListRecording) {
        submitAndAdvance();
    }

    beginRecordingIfNeeded();

    currentRtvHandle.ptr = rtvPtr;
    if (backBuffer) {
        if (rtStateResources.find(backBuffer) == rtStateResources.end()) {
            D3D12_RESOURCE_BARRIER barrier{};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = backBuffer;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            commandList->ResourceBarrier(1, &barrier);
            rtStateResources.insert(backBuffer);
        }
    }
    
    if (rtvPtr != 0) {
        commandList->OMSetRenderTargets(1, &currentRtvHandle, FALSE, nullptr);
    }
}

void dx12PreparePresent(ID3D12Resource* backBuffer)
{
    if (commandListRecording) {
        if (backBuffer) {
            if (rtStateResources.find(backBuffer) != rtStateResources.end()) {
                D3D12_RESOURCE_BARRIER barrier{};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = backBuffer;
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                commandList->ResourceBarrier(1, &barrier);
                rtStateResources.erase(backBuffer);
            }
        }
        submitAndAdvance();
    }
}

// -------------------------------------------------------------------------
// initialization & shutdown
// -------------------------------------------------------------------------

bool initDx12(Services& services)
{
	printf("Initializing HorusUI Direct3D 12 provider...\n");

	if (!device)
	{
#if defined(_DEBUG)
		ID3D12Debug* debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();
			debugController->Release();
		}
#endif
		IDXGIFactory4* factory = nullptr;
		CreateDXGIFactory1(IID_PPV_ARGS(&factory));
		IDXGIAdapter1* adapter = nullptr;
		for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
			if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_dx12Device)))) break;
			adapter->Release(); adapter = nullptr;
		}
		if (g_dx12Device) {
			D3D12_COMMAND_QUEUE_DESC queueDesc{};
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			g_dx12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_dx12CommandQueue));
			g_dx12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_dx12CommandAllocator));
			g_dx12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_dx12CommandAllocator, nullptr, IID_PPV_ARGS(&g_dx12CommandList));
			g_dx12CommandList->Close();
            
            for (int i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i) {
                g_dx12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frames[i].commandAllocator));
                g_frames[i].fenceValue = 0;
            }
            g_currentFrameIndex = 0;
		}
		if (adapter) adapter->Release();
		if (factory) factory->Release();
	}

	if (!device)
	{
		printf("Direct3D 12 device creation failed!\n");
		return false;
	}

	// 1. Root Signature
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
	srvHeapDesc.NumDescriptors = 10000;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap));

	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&uploadAllocator));
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, uploadAllocator, nullptr, IID_PPV_ARGS(&uploadCmdList));
	uploadCmdList->Close();

	// create a 1x1 white default texture for index 0 (used when no texture is bound during draw)
	{
		Dx12Texture* whiteTex = new Dx12Texture();
		whiteTex->srvHeapIndex = 0;
		
		D3D12_RESOURCE_DESC whiteDesc{};
		whiteDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		whiteDesc.Width = 1; whiteDesc.Height = 1;
		whiteDesc.DepthOrArraySize = 1; whiteDesc.MipLevels = 1;
		whiteDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		whiteDesc.SampleDesc.Count = 1;
		whiteDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		whiteDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		
		D3D12_HEAP_PROPERTIES whiteHeap{}; whiteHeap.Type = D3D12_HEAP_TYPE_DEFAULT;
		device->CreateCommittedResource(&whiteHeap, D3D12_HEAP_FLAG_NONE, &whiteDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&whiteTex->handle));
		
		D3D12_SHADER_RESOURCE_VIEW_DESC whiteSrvDesc{};
		whiteSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		whiteSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		whiteSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		whiteSrvDesc.Texture2D.MipLevels = 1;
		device->CreateShaderResourceView(whiteTex->handle, &whiteSrvDesc,
			srvHeap->GetCPUDescriptorHandleForHeapStart());
			
		// Upload 1x1 white pixel
		Rgba32 white = 0xFFFFFFFF;
		whiteTex->resize(1,1);
		whiteTex->updateData(&white);
		delete whiteTex;
	}


	D3D12_DESCRIPTOR_RANGE range{};
	range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range.NumDescriptors = 1;
	range.BaseShaderRegister = 0;
	range.RegisterSpace = 0;
	range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[2]{};
	// parameter 0: root constants for MVP
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParameters[0].Constants.ShaderRegister = 0;
	rootParameters[0].Constants.RegisterSpace = 0;
	rootParameters[0].Constants.Num32BitValues = 16;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// parameter 1: descriptor table for Texture SRV
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[1].DescriptorTable.pDescriptorRanges = &range;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// static sampler
	D3D12_STATIC_SAMPLER_DESC sampler{};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = 2;
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &sampler;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;
	if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
		if (error) { printf("Root Signature Error: %s\n", (char*)error->GetBufferPointer()); error->Release(); }
		return false;
	}
	device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	signature->Release();

	// 2. Shaders
	const char* shaderSource = R"(
cbuffer constants : register(b0) {
    matrix mvp;
};
struct VS_INPUT {
    float2 pos : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};
PS_INPUT VSMain(VS_INPUT input) {
    PS_INPUT output;
    output.pos = mul(mvp, float4(input.pos, 0.0f, 1.0f));
    output.uv  = input.uv;
    output.color = input.color;
    return output;
}
Texture2D tex : register(t0);
SamplerState smp : register(s0);
float4 PSMain(PS_INPUT input) : SV_TARGET {
    return input.color * tex.Sample(smp, input.uv);
}
)";

	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* psBlob = nullptr;
	D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, &error);
	if (error) { printf("VS Compile Error: %s\n", (char*)error->GetBufferPointer()); error->Release(); error = nullptr; }
	D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, &error);
	if (error) { printf("PS Compile Error: %s\n", (char*)error->GetBufferPointer()); error->Release(); }

	// 3. Pipeline State
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { inputLayout, 3 };
	psoDesc.pRootSignature = rootSignature;
	psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
	
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
	psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;
	psoDesc.RasterizerState.MultisampleEnable = FALSE;
	psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	psoDesc.RasterizerState.ForcedSampleCount = 0;
	psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	
	psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
	psoDesc.BlendState.IndependentBlendEnable = FALSE;
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
		psoDesc.BlendState.RenderTarget[i].BlendEnable = FALSE;
		psoDesc.BlendState.RenderTarget[i].LogicOpEnable = FALSE;
		psoDesc.BlendState.RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[i].DestBlend = D3D12_BLEND_ZERO;
		psoDesc.BlendState.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
		psoDesc.BlendState.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
		psoDesc.BlendState.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		psoDesc.BlendState.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
		psoDesc.BlendState.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}

	psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
	vsBlob->Release();
	psBlob->Release();

	// hook into Horus UI Services
	services.setViewport = setViewport;
	services.clearBackbuffer = clearBackbuffer;
	services.draw = draw;
	services.getGfxApiName = []() -> const char* { return "Direct3D 12"; };

	printf("Direct3D 12 backend initialized successfully.\n");
	return true;
}

void shutdownDx12(Services& services)
{
    flushDX12Queue();

    for (int i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i) {
        g_frames[i].destroy();
        g_frames[i].fenceValue = 0;
    }

	if (rootSignature) { rootSignature->Release(); rootSignature = nullptr; }
	if (pipelineState) { pipelineState->Release(); pipelineState = nullptr; }

	if (globalFence) { globalFence->Release(); globalFence = nullptr; }
	if (fenceEvent) { CloseHandle(fenceEvent); fenceEvent = nullptr; }

	if (srvHeap) { srvHeap->Release(); srvHeap = nullptr; }
	if (uploadCmdList) { uploadCmdList->Release(); uploadCmdList = nullptr; }
	if (uploadAllocator) { uploadAllocator->Release(); uploadAllocator = nullptr; }

	if (commandList)
	{
		commandList->Release();
		commandList = nullptr;
	}

	if (commandAllocator)
	{
		commandAllocator->Release();
		commandAllocator = nullptr;
	}

	if (commandQueue)
	{
		commandQueue->Release();
		commandQueue = nullptr;
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
