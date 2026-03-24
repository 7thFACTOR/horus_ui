#include "vulkan_graphics.h"
#include <vulkan/vulkan.h>
#include <iostream>
#include <stdexcept>
#include <string.h>
#include <SDL3/SDL_vulkan.h>
#include <vector>
#include <map>
#include <fstream>
#include <array>
#include <cstdint>
#include <cstddef>

namespace hui
{

// --- Global Vulkan State ---
static VkInstance instance = VK_NULL_HANDLE;
static VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
static VkDevice device = VK_NULL_HANDLE;
static VkQueue graphicsQueue = VK_NULL_HANDLE;
static VkCommandPool commandPool = VK_NULL_HANDLE;
static VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
static u32 graphicsQueueFamilyIndex = 0;

static Rect currentViewport;
static Color clearColor = Color::black;
static void* g_currentWindow = nullptr;

// small utility
static void checkErrorVK(VkResult result, const char* where)
{
	if (result != VK_SUCCESS)
	{
		printf("[%s] Vulkan Error: %d\n", where, result);
	}
}

// forward-declare debug utils helper
static VkResult createDebugUtilsMessengerEXT(VkInstance inst, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(inst, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(inst, pCreateInfo, pAllocator, pMessenger);
	}
	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void destroyDebugUtilsMessengerEXT(VkInstance inst, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(inst, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(inst, messenger, pAllocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	(void)messageSeverity;
	(void)messageType;
	(void)pUserData;
	fprintf(stderr, "VULKAN_VALIDATION: %s\n", pCallbackData->pMessage);
	return VK_FALSE;
}

// Find memory type
uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	throw std::runtime_error("Failed to find suitable memory type!");
}

// One-time command helpers
static VkCommandBuffer beginSingleTimeCommands()
{
	if (!device || commandPool == VK_NULL_HANDLE) return VK_NULL_HANDLE;

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

static void endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	if (!device || commandBuffer == VK_NULL_HANDLE) return;

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

// Buffer creation helper
static void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	if (!device) return;

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	checkErrorVK(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer), "createBuffer vkCreateBuffer");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	checkErrorVK(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory), "createBuffer vkAllocateMemory");
	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

// Image utilities
static void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer cmd = beginSingleTimeCommands();
	if (cmd == VK_NULL_HANDLE) return;

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}

	vkCmdPipelineBarrier(
		cmd,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommands(cmd);
}

static void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkDeviceSize bufferOffset = 0, uint32_t bufferRowLength = 0, uint32_t bufferImageHeight = 0, int32_t imageOffsetX = 0, int32_t imageOffsetY = 0)
{
	VkCommandBuffer cmd = beginSingleTimeCommands();
	if (cmd == VK_NULL_HANDLE) return;

	VkBufferImageCopy region{};
	region.bufferOffset = bufferOffset;
	region.bufferRowLength = bufferRowLength;
	region.bufferImageHeight = bufferImageHeight;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { imageOffsetX, imageOffsetY, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(
		cmd,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	endSingleTimeCommands(cmd);
}

// --- VulkanTexture implementation ---
VulkanTexture::VulkanTexture(u32 newWidth, u32 newHeight, Rgba32* pixels) { resize(newWidth,newHeight); updateData(pixels); }
VulkanTexture::VulkanTexture(u32 newWidth, u32 newHeight) { resize(newWidth,newHeight); }
VulkanTexture::~VulkanTexture() { destroy(); }

void VulkanTexture::resize(u32 newWidth, u32 newHeight)
{
	if (!device) return;
	destroy();
	width = newWidth;
	height = newHeight;

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	checkErrorVK(vkCreateImage(device, &imageInfo, nullptr, &handle), "VulkanTexture::resize vkCreateImage");
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, handle, &memRequirements);
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	checkErrorVK(vkAllocateMemory(device, &allocInfo, nullptr, &memory), "VulkanTexture::resize vkAllocateMemory");
	vkBindImageMemory(device, handle, memory, 0);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = handle;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	checkErrorVK(vkCreateImageView(device, &viewInfo, nullptr, &view), "VulkanTexture::resize vkCreateImageView");
}

void VulkanTexture::updateData(Rgba32* pixels)
{
	if (!device || !pixels || width == 0 || height == 0) return;

	VkDeviceSize imageSize = (VkDeviceSize)width * (VkDeviceSize)height * sizeof(Rgba32);

	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
	createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, (size_t)imageSize);
	vkUnmapMemory(device, stagingBufferMemory);

	transitionImageLayout(handle, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer, handle, width, height);
	transitionImageLayout(handle, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	if (stagingBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, stagingBuffer, nullptr);
	if (stagingBufferMemory != VK_NULL_HANDLE) vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void VulkanTexture::updateRectData(const Rect& rect, Rgba32* pixels)
{
	if (!device || !pixels || width == 0 || height == 0) return;

	u32 x = (u32)rect.x;
	u32 y = (u32)rect.y;
	u32 w = (u32)rect.width;
	u32 h = (u32)rect.height;

	if (x + w > width || y + h > height)
	{
		if (x >= width || y >= height) return;
		if (x + w > width) w = width - x;
		if (y + h > height) h = height - y;
	}

	VkDeviceSize regionSize = (VkDeviceSize)w * (VkDeviceSize)h * sizeof(Rgba32);

	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
	createBuffer(regionSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, regionSize, 0, &data);
	memcpy(data, pixels, (size_t)regionSize);
	vkUnmapMemory(device, stagingBufferMemory);

	transitionImageLayout(handle, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	copyBufferToImage(stagingBuffer, handle, w, h, 0, 0, 0, (int32_t)x, (int32_t)y);

	transitionImageLayout(handle, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	if (stagingBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, stagingBuffer, nullptr);
	if (stagingBufferMemory != VK_NULL_HANDLE) vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void VulkanTexture::destroy()
{
	if (!device) return;

	if (view != VK_NULL_HANDLE)
	{
		vkDestroyImageView(device, view, nullptr);
		view = VK_NULL_HANDLE;
	}
	if (handle != VK_NULL_HANDLE)
	{
		vkDestroyImage(device, handle, nullptr);
		handle = VK_NULL_HANDLE;
	}
	if (memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(device, memory, nullptr);
		memory = VK_NULL_HANDLE;
	}
	width = height = 0;
}

// VulkanVertexBuffer
VulkanVertexBuffer::VulkanVertexBuffer() {}
VulkanVertexBuffer::VulkanVertexBuffer(u32 count, Vertex* vertices) { create(count); updateData(vertices,0,count); }
VulkanVertexBuffer::~VulkanVertexBuffer() { destroy(); }
void VulkanVertexBuffer::create(u32 count) { destroy(); resize(count); }
void VulkanVertexBuffer::resize(u32 count)
{
	if (!device) return;
	if (count == 0) return;
	destroy();
	this->count = count;
	VkDeviceSize bufferSize = sizeof(Vertex) * count;
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	checkErrorVK(vkCreateBuffer(device, &bufferInfo, nullptr, &handle), "VulkanVertexBuffer::resize vkCreateBuffer");
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, handle, &memRequirements);
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	checkErrorVK(vkAllocateMemory(device, &allocInfo, nullptr, &memory), "VulkanVertexBuffer::resize vkAllocateMemory");
	vkBindBufferMemory(device, handle, memory, 0);
}
void VulkanVertexBuffer::updateData(Vertex* vertices, u32 startVertexIndex, u32 count)
{
	if (!device || !vertices || count == 0) return;
	VkDeviceSize bufferSize = sizeof(Vertex) * count;
	VkDeviceSize offset = sizeof(Vertex) * startVertexIndex;
	void* data;
	vkMapMemory(device, memory, offset, bufferSize, 0, &data);
	memcpy(data, &vertices[startVertexIndex], (size_t)bufferSize);
	vkUnmapMemory(device, memory);
}
void VulkanVertexBuffer::destroy()
{
	if (!device) return;
	if (handle != VK_NULL_HANDLE) { vkDestroyBuffer(device, handle, nullptr); handle = VK_NULL_HANDLE; }
	if (memory != VK_NULL_HANDLE) { vkFreeMemory(device, memory, nullptr); memory = VK_NULL_HANDLE; }
	count = 0;
}

// -------------------------------------------------------------------------
// Swapchain & pipeline per-window
// -------------------------------------------------------------------------
struct SwapchainContext
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;
	std::vector<VkFramebuffer> framebuffers;
	VkRenderPass renderPass = VK_NULL_HANDLE;
	VkExtent2D extent = { 0, 0 };
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkPipeline pipeline = VK_NULL_HANDLE;

	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	VkSampler sampler = VK_NULL_HANDLE;

	// per-swapchain synchronization (single in-flight)
	VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
	VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
	VkFence inFlightFence = VK_NULL_HANDLE;

	// command buffer to record rendering each frame
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
	u32 vertexBufferCount = 0;

	bool vSync = true;
};

static std::map<void*, SwapchainContext> g_swapchains; // keyed by SDL_Window*
static VulkanTexture* g_defaultWhiteTexture = nullptr;

// Shader loader helper
static std::vector<char> readFileBytes(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open()) return {};
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}

// Embedded SPIR-V for ui.vert and ui.frag (single definitions)
static const uint32_t ui_vert_spv[] = {
    0x07230203,0x00010000,0x000d000b,0x00000031,0x00000000,0x00020011,0x00000001,0x0006000b,0x00000001,0x4c534c47,
    0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,0x000b000f,0x00000000,0x00000004,0x6e69616d,
    0x00000000,0x0000000b,0x00000021,0x0000002a,0x0000002b,0x0000002d,0x0000002f,0x00030003,0x00000002,0x000001c2,
    0x000a0004,0x475f4c47,0x4c474f4f,0x70635f45,0x74735f70,0x5f656c79,0x656e696c,0x7269645f,0x69746365,0x00006576,
    0x00080004,0x475f4c47,0x4c474f4f,0x6e695f45,0x64756c63,0x69645f65,0x74636572,0x00657669,0x00040005,0x00000004,
    0x6e69616d,0x00000000,0x00030005,0x00000009,0x0063646e,0x00050005,0x0000000b,0x6f506e69,0x69746973,0x00006e6f,
    0x00040005,0x0000000d,0x68737550,0x00000000,0x00060006,0x0000000d,0x00000000,0x77656976,0x74726f70,0x00000000,
    0x00050006,0x0000000d,0x00000001,0x6d6d7564,0x00000079,0x00030005,0x0000000f,0x00006370,0x00060005,0x0000001f,
    0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x0000001f,0x00000000,0x505f6c67,0x7469736f,0x006e6f69,
    0x00070006,0x0000001f,0x00000001,0x505f6c67,0x746e696f,0x657a6953,0x00000000,0x00070006,0x0000001f,0x00000002,
    0x435f6c67,0x4470696c,0x61747369,0x0065636e,0x00070006,0x0000001f,0x00000003,0x435f6c67,0x446c6c75,0x61747369,
    0x0065636e,0x00030005,0x00000021,0x00000000,0x00030005,0x0000002a,0x00765576,0x00040005,0x0000002b,0x76556e69,
    0x00000000,0x00040005,0x0000002d,0x6c6f4376,0x0000726f,0x00040005,0x0000002f,0x6f436e69,0x00726f6c,0x00040047,
    0x0000000b,0x0000001e,0x00000000,0x00030047,0x0000000d,0x00000002,0x00050048,0x0000000d,0x00000000,0x00000023,
    0x00000000,0x00050048,0x0000000d,0x00000001,0x00000023,0x00000008,0x00030047,0x0000001f,0x00000002,0x00050048,
    0x0000001f,0x00000000,0x0000000b,0x00000000,0x00050048,0x0000001f,0x00000001,0x0000000b,0x00000001,0x00050048,
    0x0000001f,0x00000002,0x0000000b,0x00000003,0x00050048,0x0000001f,0x00000003,0x0000000b,0x00000004,0x00040047,
    0x0000002a,0x0000001e,0x00000000,0x00040047,0x0000002b,0x0000001e,0x00000001,0x00040047,0x0000002d,0x0000001e,
    0x00000001,0x00040047,0x0000002f,0x0000001e,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
    0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000002,0x00040020,0x00000008,0x00000007,
    0x00000007,0x00040020,0x0000000a,0x00000001,0x00000007,0x0004003b,0x0000000a,0x0000000b,0x00000001,0x0004001e,
    0x0000000d,0x00000007,0x00000007,0x00040020,0x0000000e,0x00000009,0x0000000d,0x0004003b,0x0000000e,0x0000000f,
    0x00000009,0x00040015,0x00000010,0x00000020,0x00000001,0x0004002b,0x00000010,0x00000011,0x00000000,0x00040020,
    0x00000012,0x00000009,0x00000007,0x0004002b,0x00000006,0x00000016,0x40000000,0x0004002b,0x00000006,0x00000018,
    0x3f800000,0x00040017,0x0000001b,0x00000006,0x00000004,0x00040015,0x0000001c,0x00000020,0x00000000,0x0004002b,
    0x0000001c,0x0000001d,0x00000001,0x0004001c,0x0000001e,0x00000006,0x0000001d,0x0006001e,0x0000001f,0x0000001b,
    0x00000006,0x0000001e,0x0000001e,0x00040020,0x00000020,0x00000003,0x0000001f,0x0004003b,0x00000020,0x00000021,
    0x00000003,0x0004002b,0x00000006,0x00000023,0x00000000,0x00040020,0x00000027,0x00000003,0x0000001b,0x00040020,
    0x00000029,0x00000003,0x00000007,0x0004003b,0x00000029,0x0000002a,0x00000003,0x0004003b,0x0000000a,0x0000002b,
    0x00000001,0x0004003b,0x00000027,0x0000002d,0x00000003,0x00040020,0x0000002e,0x00000001,0x0000001b,0x0004003b,
    0x0000002e,0x0000002f,0x00000001,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,
    0x0004003b,0x00000008,0x00000009,0x00000007,0x0004003d,0x00000007,0x0000000c,0x0000000b,0x00050041,0x00000012,
    0x00000013,0x0000000f,0x00000011,0x0004003d,0x00000007,0x00000014,0x00000013,0x00050088,0x00000007,0x00000015,
    0x0000000c,0x00000014,0x0005008e,0x00000007,0x00000017,0x00000015,0x00000016,0x00050050,0x00000007,0x00000019,
    0x00000018,0x00000018,0x00050083,0x00000007,0x0000001a,0x00000017,0x00000019,0x0003003e,0x00000009,0x0000001a,
    0x0004003d,0x00000007,0x00000022,0x00000009,0x00050051,0x00000006,0x00000024,0x00000022,0x00000000,0x00050051,
    0x00000006,0x00000025,0x00000022,0x00000001,0x00070050,0x0000001b,0x00000026,0x00000024,0x00000025,0x00000023,
    0x00000018,0x00050041,0x00000027,0x00000028,0x00000021,0x00000011,0x0003003e,0x00000028,0x00000026,0x0004003d,
    0x00000007,0x0000002c,0x0000002b,0x0003003e,0x0000002a,0x0000002c,0x0004003d,0x0000001b,0x00000030,0x0000002f,
    0x0003003e,0x0000002d,0x00000030,0x000100fd,0x00010038
};

static const size_t ui_vert_spv_size = sizeof(ui_vert_spv);

static const uint32_t ui_frag_spv[] = {
    0x07230203,0x00010000,0x0008000b,0x0000001b,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x0008000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000011,0x00000015,0x00000018,
    0x00030010,0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,
    0x6e69616d,0x00000000,0x00030005,0x00000009,0x00786574,0x00040005,0x0000000d,0x78655475,
    0x00000000,0x00030005,0x00000011,0x00765576,0x00050005,0x00000015,0x4374756f,0x726f6c6f,
    0x00000000,0x00040005,0x00000018,0x6c6f4376,0x0000726f,0x00040047,0x0000000d,0x00000021,
    0x00000000,0x00040047,0x0000000d,0x00000022,0x00000000,0x00040047,0x00000011,0x0000001e,
    0x00000000,0x00040047,0x00000015,0x0000001e,0x00000000,0x00040047,0x00000018,0x0000001e,
    0x00000001,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
    0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000007,
    0x00000007,0x00090019,0x0000000a,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,
    0x00000001,0x00000000,0x0003001b,0x0000000b,0x0000000a,0x00040020,0x0000000c,0x00000000,
    0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000000,0x00040017,0x0000000f,0x00000006,
    0x00000002,0x00040020,0x00000010,0x00000001,0x0000000f,0x0004003b,0x00000010,0x00000011,
    0x00000001,0x00040020,0x00000014,0x00000003,0x00000007,0x0004003b,0x00000014,0x00000015,
    0x00000003,0x00040020,0x00000017,0x00000001,0x00000007,0x0004003b,0x00000017,0x00000018,
    0x00000001,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,
    0x0004003b,0x00000008,0x00000009,0x00000007,0x0004003d,0x0000000b,0x0000000e,0x0000000d,
    0x0004003d,0x0000000f,0x00000012,0x00000011,0x00050057,0x00000007,0x00000013,0x0000000e,
    0x00000012,0x0003003e,0x00000009,0x00000013,0x0004003d,0x00000007,0x00000016,0x00000009,
    0x0004003d,0x00000007,0x00000019,0x00000018,0x00050085,0x00000007,0x0000001a,0x00000016,
    0x00000019,0x0003003e,0x00000015,0x0000001a,0x000100fd,0x00010038
};
static const size_t ui_frag_spv_size = sizeof(ui_frag_spv);

// Updated shader loader to attempt on-disk then embedded fallback
static bool ValidateSpirvWords(const uint32_t* pCode, size_t codeSize) noexcept {
    if (pCode == nullptr) {
        return false;
    }
    if (codeSize == 0) {
        return false;
    }
    if ((codeSize % sizeof(uint32_t)) != 0) {
        return false;
    }

    const size_t wordCount = codeSize / sizeof(uint32_t);
    if (wordCount == 0) {
        return false;
    }

    constexpr uint32_t kSpirvMagic = 0x07230203u;
    if (pCode[0] != kSpirvMagic) {
        return false;
    }

    if (wordCount > 4) {
        const uint32_t declaredBound = pCode[3];
        if (declaredBound > 10'000'000u) {
            return false;
        }
    }

    return true;
}

static VkShaderModule createShaderModuleFromFileOrEmbedded(const std::string& path, const uint32_t* embeddedWords, size_t embeddedSize)
{
    // Try file first
    auto bytes = readFileBytes(path);
    if (!bytes.empty())
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = bytes.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(bytes.data());
        VkShaderModule module = VK_NULL_HANDLE;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS) {
            printf("Failed to create shader module from %s\n", path.c_str());
            return VK_NULL_HANDLE;
        }
        return module;
    }

    // fallback to embedded
    if (embeddedWords != nullptr && embeddedSize > 0)
    {
        // Validate embedded SPIR-V before calling vkCreateShaderModule
        if (!ValidateSpirvWords(embeddedWords, embeddedSize)) {
            printf("Embedded SPIR-V for %s failed basic validation\n", path.c_str());
            return VK_NULL_HANDLE;
        }

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = embeddedSize;
        createInfo.pCode = embeddedWords;

        VkShaderModule module = VK_NULL_HANDLE;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS) {
            printf("Failed to create shader module from embedded data for %s\n", path.c_str());
            return VK_NULL_HANDLE;
        }
        return module;
    }

    // nothing available
    return VK_NULL_HANDLE;
}

// Pipeline creation helper (textured pipeline)
static bool createPipelineForSwapchain(SwapchainContext& ctx)
{
	// load shader modules (from disk or embedded)
	auto vertModule = createShaderModuleFromFileOrEmbedded("shaders/ui.vert.spv", ui_vert_spv, ui_vert_spv_size);
	auto fragModule = createShaderModuleFromFileOrEmbedded("shaders/ui.frag.spv", ui_frag_spv, ui_frag_spv_size);
	if (vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE)
	{
		if (vertModule != VK_NULL_HANDLE) vkDestroyShaderModule(device, vertModule, nullptr);
		if (fragModule != VK_NULL_HANDLE) vkDestroyShaderModule(device, fragModule, nullptr);
		printf("Shaders not available, cannot create pipeline\n");
		return false;
	}

	// shader stages
	VkPipelineShaderStageCreateInfo vertStage{};
	vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertStage.module = vertModule;
	vertStage.pName = "main";

	VkPipelineShaderStageCreateInfo fragStage{};
	fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStage.module = fragModule;
	fragStage.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertStage, fragStage };

	// vertex input (match horus::Vertex)
	VkVertexInputBindingDescription bindingDesc{};
	bindingDesc.binding = 0;
	bindingDesc.stride = sizeof(Vertex);
	bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::array<VkVertexInputAttributeDescription, 3> attr{};
	attr[0].binding = 0; attr[0].location = 0; attr[0].format = VK_FORMAT_R32G32_SFLOAT; attr[0].offset = offsetof(Vertex, position);
	attr[1].binding = 0; attr[1].location = 1; attr[1].format = VK_FORMAT_R32G32_SFLOAT; attr[1].offset = offsetof(Vertex, uv);
	attr[2].binding = 0; attr[2].location = 2; attr[2].format = VK_FORMAT_R8G8B8A8_UNORM; attr[2].offset = offsetof(Vertex, color);

	VkPipelineVertexInputStateCreateInfo vi{};
	vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vi.vertexBindingDescriptionCount = 1;
	vi.pVertexBindingDescriptions = &bindingDesc;
	vi.vertexAttributeDescriptionCount = (uint32_t)attr.size();
	vi.pVertexAttributeDescriptions = attr.data();

	VkPipelineInputAssemblyStateCreateInfo ia{};
	ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	ia.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)ctx.extent.width;
	viewport.height = (float)ctx.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = {0,0};
	scissor.extent = ctx.extent;

	VkPipelineViewportStateCreateInfo vp{};
	vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vp.viewportCount = 1;
	vp.pViewports = &viewport;
	vp.scissorCount = 1;
	vp.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rs{};
	rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rs.depthClampEnable = VK_FALSE;
	rs.rasterizerDiscardEnable = VK_FALSE;
	rs.polygonMode = VK_POLYGON_MODE_FILL;
	rs.cullMode = VK_CULL_MODE_NONE;
	rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rs.depthBiasEnable = VK_FALSE;
	rs.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo ms{};
	ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	ms.sampleShadingEnable = VK_FALSE;
	ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState att{};
	att.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	att.blendEnable = VK_TRUE;
	att.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	att.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	att.colorBlendOp = VK_BLEND_OP_ADD;
	att.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	att.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	att.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo cb{};
	cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	cb.attachmentCount = 1;
	cb.pAttachments = &att;

	// Descriptor set layout: combined image sampler at set0 binding0
	VkDescriptorSetLayoutBinding samplerBinding{};
	samplerBinding.binding = 0;
	samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerBinding.descriptorCount = 1;
	samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo dslci{};
	dslci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	dslci.bindingCount = 1;
	dslci.pBindings = &samplerBinding;

	if (ctx.descriptorSetLayout == VK_NULL_HANDLE)
		vkCreateDescriptorSetLayout(device, &dslci, nullptr, &ctx.descriptorSetLayout);

	// pipeline layout with push constants for viewport (vec4) if needed
	VkPushConstantRange pushRange{};
	pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushRange.offset = 0;
	pushRange.size = sizeof(float) * 4; // px->NDC transform (width,height)

	VkPipelineLayoutCreateInfo plc{};
	plc.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	plc.setLayoutCount = 1;
	plc.pSetLayouts = &ctx.descriptorSetLayout;
	plc.pushConstantRangeCount = 1;
	plc.pPushConstantRanges = &pushRange;

	if (ctx.pipelineLayout == VK_NULL_HANDLE)
		vkCreatePipelineLayout(device, &plc, nullptr, &ctx.pipelineLayout);

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vi;
	pipelineInfo.pInputAssemblyState = &ia;
	pipelineInfo.pViewportState = &vp;
	pipelineInfo.pRasterizationState = &rs;
	pipelineInfo.pMultisampleState = &ms;
	pipelineInfo.pColorBlendState = &cb;
	pipelineInfo.layout = ctx.pipelineLayout;
	pipelineInfo.renderPass = ctx.renderPass;
	pipelineInfo.subpass = 0;

	VkPipeline pipe = VK_NULL_HANDLE;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipe) != VK_SUCCESS)
	{
		printf("Failed to create graphics pipeline\n");
		vkDestroyShaderModule(device, vertModule, nullptr);
		vkDestroyShaderModule(device, fragModule, nullptr);
		return false;
	}
	ctx.pipeline = pipe;

	// cleanup shader modules
	vkDestroyShaderModule(device, vertModule, nullptr);
	vkDestroyShaderModule(device, fragModule, nullptr);

	// create a sampler if not present
	if (ctx.sampler == VK_NULL_HANDLE)
	{
		VkSamplerCreateInfo sci{};
		sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sci.magFilter = VK_FILTER_LINEAR;
		sci.minFilter = VK_FILTER_LINEAR;
		sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sci.anisotropyEnable = VK_FALSE;
		sci.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
		sci.unnormalizedCoordinates = VK_FALSE;
		sci.compareEnable = VK_FALSE;
		sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		checkErrorVK(vkCreateSampler(device, &sci, nullptr, &ctx.sampler), "create sampler");
	}

	// create descriptor pool for one combined image sampler
	if (ctx.descriptorPool == VK_NULL_HANDLE)
	{
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize.descriptorCount = 1;

		VkDescriptorPoolCreateInfo dpci{};
		dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		dpci.poolSizeCount = 1;
		dpci.pPoolSizes = &poolSize;
		dpci.maxSets = 1;

		checkErrorVK(vkCreateDescriptorPool(device, &dpci, nullptr, &ctx.descriptorPool), "create descriptor pool");
	}

	return true;
}

// create render pass
static bool createRenderPassForSwapchain(SwapchainContext& ctx)
{
	if (ctx.renderPass != VK_NULL_HANDLE) return true;

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = ctx.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo rpci{};
	rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rpci.attachmentCount = 1;
	rpci.pAttachments = &colorAttachment;
	rpci.subpassCount = 1;
	rpci.pSubpasses = &subpass;

	if (vkCreateRenderPass(device, &rpci, nullptr, &ctx.renderPass) != VK_SUCCESS)
	{
		printf("Failed to create render pass\n");
		return false;
	}
	return true;
}

// create framebuffers for swapchain images
static bool createFramebuffersForSwapchain(SwapchainContext& ctx)
{
	ctx.framebuffers.resize(ctx.imageViews.size());
	for (size_t i = 0; i < ctx.imageViews.size(); ++i)
	{
		VkImageView attachments[] = { ctx.imageViews[i] };
		VkFramebufferCreateInfo fci{};
		fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fci.renderPass = ctx.renderPass;
		fci.attachmentCount = 1;
		fci.pAttachments = attachments;
		fci.width = ctx.extent.width;
		fci.height = ctx.extent.height;
		fci.layers = 1;
		if (vkCreateFramebuffer(device, &fci, nullptr, &ctx.framebuffers[i]) != VK_SUCCESS)
		{
			printf("Failed to create framebuffer\n");
			return false;
		}
	}
	return true;
}

// helper to create swapchain for a surface (basic)
bool createSwapchainForWindow(void* sdlWindow, VkSurfaceKHR surface, u32 width, u32 height, bool vSync)
{
	if (!isVulkanInitialized()) return false;

	SwapchainContext ctx{};
	ctx.surface = surface;
	ctx.extent = { (uint32_t)width, (uint32_t)height };
	ctx.format = VK_FORMAT_R8G8B8A8_UNORM;

	// query surface capabilities & present modes
	VkSurfaceCapabilitiesKHR caps{};
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &caps) != VK_SUCCESS)
	{
		printf("Failed to query surface capabilities\n");
		return false;
	}

	// choose present mode
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // FIFO = vsync
	// create swapchain
	VkSwapchainCreateInfoKHR sci{};
	sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	sci.surface = surface;
	sci.minImageCount = std::max<uint32_t>(2, caps.minImageCount);
	sci.imageFormat = ctx.format;
	sci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	sci.imageExtent = ctx.extent;
	sci.imageArrayLayers = 1;
	sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	sci.preTransform = caps.currentTransform;
	sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	sci.presentMode = presentMode;
	sci.clipped = VK_TRUE;
	sci.oldSwapchain = VK_NULL_HANDLE;
	if (vkCreateSwapchainKHR(device, &sci, nullptr, &ctx.swapchain) != VK_SUCCESS)
	{
		printf("Failed to create swapchain\n");
		return false;
	}

	// get images
	uint32_t imageCount = 0;
	vkGetSwapchainImagesKHR(device, ctx.swapchain, &imageCount, nullptr);
	ctx.images.resize(imageCount);
	vkGetSwapchainImagesKHR(device, ctx.swapchain, &imageCount, ctx.images.data());

	// create image views
	ctx.imageViews.resize(ctx.images.size());
	for (size_t i = 0; i < ctx.images.size(); ++i)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = ctx.images[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = ctx.format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		if (vkCreateImageView(device, &viewInfo, nullptr, &ctx.imageViews[i]) != VK_SUCCESS)
		{
			printf("Failed to create image view\n");
			return false;
		}
	}

	// create render pass
	if (!createRenderPassForSwapchain(ctx)) return false;

	// create framebuffers
	if (!createFramebuffersForSwapchain(ctx)) return false;

	// allocate one command buffer
	VkCommandBufferAllocateInfo cbai{};
	cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbai.commandPool = commandPool;
	cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cbai.commandBufferCount = 1;
	vkAllocateCommandBuffers(device, &cbai, &ctx.commandBuffer);

	// create sync objects
	VkSemaphoreCreateInfo sciInfo{};
	sciInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(device, &sciInfo, nullptr, &ctx.imageAvailableSemaphore);
	vkCreateSemaphore(device, &sciInfo, nullptr, &ctx.renderFinishedSemaphore);

	VkFenceCreateInfo fci{};
	fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	vkCreateFence(device, &fci, nullptr, &ctx.inFlightFence);

	// create pipeline & descriptor layouts (requires shaders)
	if (!createPipelineForSwapchain(ctx))
	{
		// pipeline creation failed (likely shaders missing) -> still keep swapchain though
		printf("Pipeline creation failed for swapchain; rendering will be disabled until shaders are available.\n");
	}

	// store vertex buffer placeholders (none yet)
	ctx.vertexBuffer = VK_NULL_HANDLE;
	ctx.vertexBufferMemory = VK_NULL_HANDLE;
	ctx.vertexBufferCount = 0;

	ctx.vSync = vSync;
	g_swapchains[sdlWindow] = ctx;
	return true;
}

void destroySwapchainForWindow(void* sdlWindow)
{
	auto it = g_swapchains.find(sdlWindow);
	if (it == g_swapchains.end()) return;
	SwapchainContext& ctx = it->second;
	vkDeviceWaitIdle(device);

	if (ctx.vertexBuffer != VK_NULL_HANDLE) { vkDestroyBuffer(device, ctx.vertexBuffer, nullptr); ctx.vertexBuffer = VK_NULL_HANDLE; }
	if (ctx.vertexBufferMemory != VK_NULL_HANDLE) { vkFreeMemory(device, ctx.vertexBufferMemory, nullptr); ctx.vertexBufferMemory = VK_NULL_HANDLE; }

	if (ctx.pipeline != VK_NULL_HANDLE) { vkDestroyPipeline(device, ctx.pipeline, nullptr); ctx.pipeline = VK_NULL_HANDLE; }
	if (ctx.pipelineLayout != VK_NULL_HANDLE) { vkDestroyPipelineLayout(device, ctx.pipelineLayout, nullptr); ctx.pipelineLayout = VK_NULL_HANDLE; }
	if (ctx.descriptorPool != VK_NULL_HANDLE) { vkDestroyDescriptorPool(device, ctx.descriptorPool, nullptr); ctx.descriptorPool = VK_NULL_HANDLE; }
	if (ctx.descriptorSetLayout != VK_NULL_HANDLE) { vkDestroyDescriptorSetLayout(device, ctx.descriptorSetLayout, nullptr); ctx.descriptorSetLayout = VK_NULL_HANDLE; }
	if (ctx.sampler != VK_NULL_HANDLE) { vkDestroySampler(device, ctx.sampler, nullptr); ctx.sampler = VK_NULL_HANDLE; }
	
	if (ctx.commandBuffer != VK_NULL_HANDLE) { vkFreeCommandBuffers(device, commandPool, 1, &ctx.commandBuffer); ctx.commandBuffer = VK_NULL_HANDLE; }
	if (ctx.renderFinishedSemaphore != VK_NULL_HANDLE) { vkDestroySemaphore(device, ctx.renderFinishedSemaphore, nullptr); ctx.renderFinishedSemaphore = VK_NULL_HANDLE; }
	if (ctx.imageAvailableSemaphore != VK_NULL_HANDLE) { vkDestroySemaphore(device, ctx.imageAvailableSemaphore, nullptr); ctx.imageAvailableSemaphore = VK_NULL_HANDLE; }
	if (ctx.inFlightFence != VK_NULL_HANDLE) { vkDestroyFence(device, ctx.inFlightFence, nullptr); ctx.inFlightFence = VK_NULL_HANDLE; }

	for (auto fb : ctx.framebuffers) vkDestroyFramebuffer(device, fb, nullptr);
	ctx.framebuffers.clear();
	if (ctx.renderPass != VK_NULL_HANDLE) { vkDestroyRenderPass(device, ctx.renderPass, nullptr); ctx.renderPass = VK_NULL_HANDLE; }
	for (auto iv : ctx.imageViews) vkDestroyImageView(device, iv, nullptr);
	ctx.imageViews.clear();

	if (ctx.swapchain != VK_NULL_HANDLE) { vkDestroySwapchainKHR(device, ctx.swapchain, nullptr); ctx.swapchain = VK_NULL_HANDLE; }

	g_swapchains.erase(it);
}

// update/create vertex buffer for context
static bool ensureVertexBuffer(SwapchainContext& ctx, u32 vertexCount)
{
	if (ctx.vertexBufferCount >= vertexCount) return true;

	// destroy old
	if (ctx.vertexBuffer != VK_NULL_HANDLE) { vkDestroyBuffer(device, ctx.vertexBuffer, nullptr); ctx.vertexBuffer = VK_NULL_HANDLE; }
	if (ctx.vertexBufferMemory != VK_NULL_HANDLE) { vkFreeMemory(device, ctx.vertexBufferMemory, nullptr); ctx.vertexBufferMemory = VK_NULL_HANDLE; }

	VkDeviceSize size = sizeof(Vertex) * vertexCount;
	createBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, ctx.vertexBuffer, ctx.vertexBufferMemory);
	ctx.vertexBufferCount = vertexCount;
	return true;
}

// Draw: store vertex data into the global per-window vertex buffer and batches
struct DrawSubmission
{
	std::vector<Vertex> vertices;
	std::vector<RenderBatch> batches;
};
static std::map<void*, DrawSubmission> g_drawSubmissions;

static void vulkanSetCurrentWindowInternal(void* wnd)
{
	g_currentWindow = wnd;
}

void vulkanSetCurrentWindow(void* sdlWindow)
{
	g_currentWindow = sdlWindow;
}

static void setViewport(const Point& windowSize, const Rect& viewport)
{
	currentViewport = viewport;
}

static void clearBackbuffer(const Color& color)
{
	clearColor = color;
}

static void draw(Vertex* vertices, u32 vertexCount, struct RenderBatch* batches, u32 batchCount)
{
	if (!device || !vertices || vertexCount == 0) return;
	void* wnd = g_currentWindow;
	auto& sub = g_drawSubmissions[wnd];
	sub.vertices.assign(vertices, vertices + vertexCount);
	sub.batches.clear();
	for (u32 i = 0; i < batchCount; ++i) sub.batches.push_back(batches[i]);
}

// present implementation
bool presentSwapchainForWindow(void* sdlWindow)
{
	auto it = g_swapchains.find(sdlWindow);
	if (it == g_swapchains.end()) return false;
	SwapchainContext& ctx = it->second;
	if (!ctx.swapchain) return false;

	vkWaitForFences(device, 1, &ctx.inFlightFence, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &ctx.inFlightFence);

	if (ctx.descriptorPool != VK_NULL_HANDLE)
		vkResetDescriptorPool(device, ctx.descriptorPool, 0);

	uint32_t imageIndex;
	VkResult res = vkAcquireNextImageKHR(device, ctx.swapchain, UINT64_MAX, ctx.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
	if (res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		vkDeviceWaitIdle(device);
		VkSurfaceKHR surface = ctx.surface;
		bool vSync = ctx.vSync;
		int w, h;
		SDL_GetWindowSizeInPixels((SDL_Window*)sdlWindow, &w, &h);
		if (w > 0 && h > 0)
		{
			destroySwapchainForWindow(sdlWindow);
			createSwapchainForWindow(sdlWindow, surface, (u32)w, (u32)h, vSync);
		}
		return false;
	}
	else if (res == VK_SUBOPTIMAL_KHR)
	{
		// ignore for now to avoid infinite loops
	}
	else if (res != VK_SUCCESS)
	{
		printf("Failed to acquire swapchain image\n");
		return false;
	}

	void* key = sdlWindow;
	auto dit = g_drawSubmissions.find(key);
	DrawSubmission* sub = (dit != g_drawSubmissions.end()) ? &dit->second : nullptr;

	if (sub && !sub->vertices.empty())
	{
		ensureVertexBuffer(ctx, (u32)sub->vertices.size());
		// copy vertex data
		void* data;
		vkMapMemory(device, ctx.vertexBufferMemory, 0, sizeof(Vertex) * sub->vertices.size(), 0, &data);
		memcpy(data, sub->vertices.data(), sizeof(Vertex) * sub->vertices.size());
		vkUnmapMemory(device, ctx.vertexBufferMemory);
	}

	// record command buffer
	VkCommandBufferBeginInfo cbbi{};
	cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cbbi.flags = 0;
	vkBeginCommandBuffer(ctx.commandBuffer, &cbbi);

	VkClearValue clearValue{};
	clearValue.color = { { clearColor.r, clearColor.g, clearColor.b, clearColor.a } };

	VkRenderPassBeginInfo rpbi{};
	rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpbi.renderPass = ctx.renderPass;
	rpbi.framebuffer = ctx.framebuffers[imageIndex];
	rpbi.renderArea.offset = {0,0};
	rpbi.renderArea.extent = ctx.extent;
	rpbi.clearValueCount = 1;
	rpbi.pClearValues = &clearValue;

	vkCmdBeginRenderPass(ctx.commandBuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

	if (sub && !sub->vertices.empty() && ctx.pipeline != VK_NULL_HANDLE)
	{
		vkCmdBindPipeline(ctx.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.pipeline);
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(ctx.commandBuffer, 0, 1, &ctx.vertexBuffer, offsets);

		float pc[4] = { (float)ctx.extent.width, (float)ctx.extent.height, 0.0f, 0.0f };
		vkCmdPushConstants(ctx.commandBuffer, ctx.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), pc);

		for (size_t b = 0; b < sub->batches.size(); ++b)
		{
			RenderBatch& rb = sub->batches[b];
			if (rb.vertexCount == 0) continue;
			VkDescriptorSet descSet = VK_NULL_HANDLE;
			if (ctx.descriptorPool != VK_NULL_HANDLE && ctx.descriptorSetLayout != VK_NULL_HANDLE)
			{
				VkDescriptorSetAllocateInfo dsai{};
				dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				dsai.descriptorPool = ctx.descriptorPool;
				dsai.descriptorSetCount = 1;
				dsai.pSetLayouts = &ctx.descriptorSetLayout;
				if (vkAllocateDescriptorSets(device, &dsai, &descSet) == VK_SUCCESS)
				{
					VkDescriptorImageInfo imgInfo{};
					imgInfo.sampler = ctx.sampler;
					VkImageView view = VK_NULL_HANDLE;
					if (rb.texture)
					{
						VulkanTexture* vt = (VulkanTexture*)rb.texture;
						if (vt && vt->getView()) view = vt->getView();
					}
					if (view == VK_NULL_HANDLE && g_defaultWhiteTexture) view = g_defaultWhiteTexture->getView();
					imgInfo.imageView = view;
					imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

					VkWriteDescriptorSet wds{};
					wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					wds.dstSet = descSet;
					wds.dstBinding = 0;
					wds.dstArrayElement = 0;
					wds.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					wds.descriptorCount = 1;
					wds.pImageInfo = &imgInfo;
					vkUpdateDescriptorSets(device, 1, &wds, 0, nullptr);
					vkCmdBindDescriptorSets(ctx.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.pipelineLayout, 0, 1, &descSet, 0, nullptr);
				}
			}

			vkCmdDraw(ctx.commandBuffer, rb.vertexCount, 1, rb.startVertexIndex, 0);
		}
	}

	vkCmdEndRenderPass(ctx.commandBuffer);
	vkEndCommandBuffer(ctx.commandBuffer);

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &ctx.imageAvailableSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &ctx.commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &ctx.renderFinishedSemaphore;

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, ctx.inFlightFence) != VK_SUCCESS)
	{
		printf("Failed to submit draw command buffer\n");
		return false;
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &ctx.renderFinishedSemaphore;
	presentInfo.swapchainCount = 1;
	VkSwapchainKHR sc = ctx.swapchain;
	presentInfo.pSwapchains = &sc;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	VkResult pres = vkQueuePresentKHR(graphicsQueue, &presentInfo);
	if (pres == VK_ERROR_OUT_OF_DATE_KHR)
	{
		vkDeviceWaitIdle(device);
		VkSurfaceKHR surface = ctx.surface;
		bool vSync = ctx.vSync;
		int w, h;
		SDL_GetWindowSizeInPixels((SDL_Window*)sdlWindow, &w, &h);
		if (w > 0 && h > 0)
		{
			destroySwapchainForWindow(sdlWindow);
			createSwapchainForWindow(sdlWindow, surface, (u32)w, (u32)h, vSync);
		}
		return false;
	}
	else if (pres == VK_SUBOPTIMAL_KHR)
	{
		// ignore for now
	}
	else if (pres != VK_SUCCESS)
	{
		printf("Failed to present swapchain image %d\n", pres);
		return false;
	}
	return true;
}

// Set default white texture handle (optional)
void setDefaultWhiteTexture(VulkanTexture* tex)
{
	g_defaultWhiteTexture = tex;
}

// ---------------- init/shutdown ----------------

bool initVulkan(Services& services)
{
	printf("Initializing HorusUI Vulkan provider...\n");

	// Application info
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Horus UI";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Horus UI Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// SDL instance extensions
	Uint32 sdlExtCount = 0;
	const char* const* sdlExts = SDL_Vulkan_GetInstanceExtensions(&sdlExtCount);
	if (!sdlExts)
	{
		printf("SDL_Vulkan_GetInstanceExtensions failed: %s\n", SDL_GetError());
		return false;
	}

	std::vector<const char*> extensions;
	for (Uint32 i = 0; i < sdlExtCount; ++i)
		extensions.push_back(sdlExts[i]);

	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	// validation layer
	const char* validationLayer = "VK_LAYER_KHRONOS_validation";
	bool enableValidation = false;
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	if (layerCount > 0)
	{
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
		for (auto& layer : availableLayers)
		{
			if (strcmp(layer.layerName, validationLayer) == 0)
			{
				enableValidation = true;
				break;
			}
		}
	}

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = (uint32_t)extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidation)
	{
		createInfo.enabledLayerCount = 1;
		createInfo.ppEnabledLayerNames = &validationLayer;

		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = debugCallback;
		debugCreateInfo.pUserData = nullptr;
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else createInfo.enabledLayerCount = 0;

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
	{
		printf("Failed to create Vulkan instance!\n");
		return false;
	}

	if (enableValidation)
	{
		if (createDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS)
		{
			printf("Failed to set up debug messenger!\n");
			debugMessenger = VK_NULL_HANDLE;
		}
	}

	// pick physical device
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0)
	{
		printf("Failed to find GPUs with Vulkan support!\n");
		return false;
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	physicalDevice = devices[0];

	// find queue family
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
	bool validQueueFound = false;
	for (uint32_t i = 0; i < queueFamilyCount; i++)
	{
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			graphicsQueueFamilyIndex = i;
			validQueueFound = true;
			break;
		}
	}
	if (!validQueueFound) { printf("Failed to find a Vulkan graphics queue family!\n"); return false; }

	// create logical device (enable swapchain extension)
	float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkPhysicalDeviceFeatures deviceFeatures{};
	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	deviceCreateInfo.enabledExtensionCount = 1;
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

	if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
	{
		printf("Failed to create Vulkan logical device!\n");
		return false;
	}

	vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		printf("Failed to create Vulkan command pool!\n");
		return false;
	}

	// Hook services
	services.setViewport = setViewport;
	services.clearBackbuffer = clearBackbuffer;
	services.draw = draw;
	services.getGfxApiName = []() -> const char* { return "Vulkan"; };

	printf("Vulkan Backend initialized successfully.\n");
	return true;
}

void shutdownVulkan(Services& services)
{
	// destroy all swapchains
	for (auto it = g_swapchains.begin(); it != g_swapchains.end(); )
	{
		destroySwapchainForWindow(it->first);
		it = g_swapchains.begin(); // restart since destroy erases
	}

	if (device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(device);

		if (commandPool != VK_NULL_HANDLE) {
			vkDestroyCommandPool(device, commandPool, nullptr);
			commandPool = VK_NULL_HANDLE;
		}

		vkDestroyDevice(device, nullptr);
		device = VK_NULL_HANDLE;
	}

	if (debugMessenger != VK_NULL_HANDLE)
	{
		destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		debugMessenger = VK_NULL_HANDLE;
	}

	if (instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(instance, nullptr);
		instance = VK_NULL_HANDLE;
	}

	services.setViewport = nullptr;
	services.clearBackbuffer = nullptr;
	services.draw = nullptr;
	services.getGfxApiName = nullptr;
}

bool isVulkanInitialized()
{
	return instance != VK_NULL_HANDLE && device != VK_NULL_HANDLE;
}

bool createSurfaceForSdlWindow(void* sdlWindowVoid, VkSurfaceKHR* outSurface)
{
	if (!isVulkanInitialized() || !sdlWindowVoid || !outSurface)
		return false;

	SDL_Window* sdlWindow = (SDL_Window*)sdlWindowVoid;

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	if (!SDL_Vulkan_CreateSurface(sdlWindow, instance, nullptr, &surface))
	{
		printf("SDL_Vulkan_CreateSurface failed: %s\n", SDL_GetError());
		return false;
	}

	VkBool32 supported = VK_FALSE;
	if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, graphicsQueueFamilyIndex, surface, &supported) != VK_SUCCESS || !supported)
	{
		printf("VkSurface is not supported by the selected physical device/queue family\n");
		SDL_Vulkan_DestroySurface(instance, surface, nullptr);
		return false;
	}

	*outSurface = surface;
	return true;
}

void destroySurface(VkSurfaceKHR surface)
{
	if (surface != VK_NULL_HANDLE && instance != VK_NULL_HANDLE)
	{
		SDL_Vulkan_DestroySurface(instance, surface, nullptr);
	}
}

// Add near your helper functions in vulkan_graphics.cpp

} // namespace hui
