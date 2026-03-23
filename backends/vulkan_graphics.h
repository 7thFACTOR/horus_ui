#pragma once
#include "horus.h"
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace hui
{
struct VulkanTexture
{
	VulkanTexture() {}
	VulkanTexture(u32 newWidth, u32 newHeight, Rgba32* pixels);
	VulkanTexture(u32 newWidth, u32 newHeight);
	~VulkanTexture();
	void destroy();
	void resize(u32 newWidth, u32 newHeight);
	void updateData(Rgba32* pixels);
	void updateRectData(const Rect& rect, Rgba32* pixels);
	HTexture getHandle() const { return (HTexture)handle; }
	VkImage getImage() const { return handle; }
	VkImageView getView() const { return view; }
	u32 getWidth() const { return width; }
	u32 getHeight() const { return height; }

	VkImage handle = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	u32 width = 0;
	u32 height = 0;
};

struct VulkanVertexBuffer
{
	VulkanVertexBuffer();
	VulkanVertexBuffer(u32 count, Vertex* vertices);
	~VulkanVertexBuffer();
	void resize(u32 count);
	void updateData(Vertex* vertices, u32 startVertexIndex, u32 count);
	void destroy();
	void create(u32 count);

	VkBuffer handle = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	u32 count = 0;
};

// initializes the Vulkan graphics backend
bool initVulkan(Services& services);

// shuts down the Vulkan graphics backend
void shutdownVulkan(Services& services);

// helper: is the Vulkan instance/device initialized
bool isVulkanInitialized();

// create/destroy a VkSurfaceKHR for an SDL_Window (returns true on success)
bool createSurfaceForSdlWindow(void* sdlWindow, VkSurfaceKHR* outSurface);
void destroySurface(VkSurfaceKHR surface);

// Create/destroy per-window swapchain & resources (surface must be valid).
// sdlWindow is the SDL_Window* pointer used as a key.
bool createSwapchainForWindow(void* sdlWindow, VkSurfaceKHR surface, u32 width, u32 height, bool vSync = true);
void destroySwapchainForWindow(void* sdlWindow);

// Present the window swapchain (acquire, draw recorded content, present)
bool presentSwapchainForWindow(void* sdlWindow);

// Optional: call to set a global default white texture for UI
void setDefaultWhiteTexture(VulkanTexture* tex);

}
