#include "vulkan_graphics.h"
#include <iostream>
#include <stdexcept>
#include <string.h>

namespace hui
{
// global Vulkan state for the backend
static VkInstance instance = VK_NULL_HANDLE;
static VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
static VkDevice device = VK_NULL_HANDLE;
static VkQueue graphicsQueue = VK_NULL_HANDLE;
static VkCommandPool commandPool = VK_NULL_HANDLE;
static u32 graphicsQueueFamilyIndex = 0;
static Rect currentViewport;

static void checkErrorVK(VkResult result, const char* where)
{
	if (result != VK_SUCCESS)
	{
		printf("[%s] Vulkan Error: %d\n", where, result);
	}
}

// memory allocation helper
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

// -------------------------------------------------------------------------
// VulkanTexture Implementation
// -------------------------------------------------------------------------

VulkanTexture::VulkanTexture(u32 newWidth, u32 newHeight, Rgba32* pixels)
{
	resize(newWidth, newHeight);
	updateData(pixels);
}

VulkanTexture::VulkanTexture(u32 newWidth, u32 newHeight)
{
	resize(newWidth, newHeight);
}

VulkanTexture::~VulkanTexture()
{
	destroy();
}

void VulkanTexture::resize(u32 newWidth, u32 newHeight)
{
	if (!device) return;

	destroy();

	width = newWidth;
	height = newHeight;

	// create image
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

	// allocate memory
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, handle, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	checkErrorVK(vkAllocateMemory(device, &allocInfo, nullptr, &memory), "VulkanTexture::resize vkAllocateMemory");
	vkBindImageMemory(device, handle, memory, 0);

	// create image view
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
	if (!device || !pixels) return;
	// in a complete implementation, this would:
	// 1. create a staging buffer
	// 2. map memory and copy pixels to staging buffer
	// 3. command buffer to transition image to DST_OPTIMAL
	// 4. copy buffer to image
	// 5. command buffer to transition image to SHADER_READ_ONLY_OPTIMAL
	// 6. submit and wait
	printf("VulkanTexture::updateData not fully implemented (requires staging buffer)\n");
}

void VulkanTexture::updateRectData(const Rect& rect, Rgba32* pixels)
{
	if (!device || !pixels) return;
	// similar to updateData but updating a specific region copy
	printf("VulkanTexture::updateRectData not fully implemented (requires staging buffer)\n");
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
}

// -------------------------------------------------------------------------
// VulkanVertexBuffer Implementation
// -------------------------------------------------------------------------

VulkanVertexBuffer::VulkanVertexBuffer() {}

VulkanVertexBuffer::VulkanVertexBuffer(u32 count, Vertex* vertices)
{
	create(count);
	updateData(vertices, 0, count);
}

VulkanVertexBuffer::~VulkanVertexBuffer()
{
	destroy();
}

void VulkanVertexBuffer::create(u32 count)
{
	destroy();
	resize(count);
}

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
	// used as vertex buffer, and transfer destination for updates
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	checkErrorVK(vkCreateBuffer(device, &bufferInfo, nullptr, &handle), "VulkanVertexBuffer::resize vkCreateBuffer");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, handle, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	// for frequent updates, host visible memory is easier, but ideally staging buffers to device local memory is best.
	// for simplicity in UI rendering, HOST_VISIBLE | HOST_COHERENT is often okay.
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

	if (handle != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(device, handle, nullptr);
		handle = VK_NULL_HANDLE;
	}
	if (memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(device, memory, nullptr);
		memory = VK_NULL_HANDLE;
	}
	count = 0;
}

// -------------------------------------------------------------------------
// UI Rendering Services Callbacks
// -------------------------------------------------------------------------

static void setViewport(const Point& windowSize, const Rect& viewport)
{
	currentViewport = viewport;
	// in Vulkan, viewport is set within the command buffer during recording:
	// vkCmdSetViewport(commandBuffer, 0, 1, &vkViewport);
	// we cache it here to use during draw() if dynamic viewport state is enabled.
}

static void clearBackbuffer(const Color& color)
{
	// in Vulkan, clear comes inherently within the render pass begin info,
	// or vkCmdClearColorImage if outside render pass.
	// for Horus UI, clear logic would be applied to the current frame render pass.
}

static void draw(Vertex* vertices, u32 vertexCount, struct RenderBatch* batches, u32 count)
{
	if (!device) return;

	// in a complete Vulkan implementation, this function would:
	// 1. update the VertexBuffer with `vertices` data.
	// 2. begin a CommandBuffer recording
	// 3. begin RenderPass
	// 4. bind Pipeline
	// 5. bind VertexBuffer
	// 6. iterate batches:
	//    a. update Descriptor Sets for texture / MVP matrix (Push Constants)
	//    b. vkCmdDraw
	// 7. end RenderPass
	// 8. end CommandBuffer and Submit to Queue

	// for now, it represents the hook where rendering would execute.
}

// -------------------------------------------------------------------------
// Initialization & Shutdown
// -------------------------------------------------------------------------

bool initVulkan(Services& services)
{
	printf("Initializing HorusUI Vulkan provider...\n");

	// 1. create instance
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Horus UI";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Horus UI Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// extension handling goes here (e.g. VK_KHR_SURFACE)
	// createInfo.enabledExtensionCount = ...
	// createInfo.ppEnabledExtensionNames = ...

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
	{
		printf("Failed to create Vulkan instance!\n");
		return false;
	}

	// 2. select physical device
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0)
	{
		printf("Failed to find GPUs with Vulkan support!\n");
		return false;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	
	physicalDevice = devices[0]; // naively selecting the first one

	// 3. find queue families
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	bool validQueueFound = false;
	for (uint32_t i = 0; i < queueFamilyCount; i++) {
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			graphicsQueueFamilyIndex = i;
			validQueueFound = true;
			break;
		}
	}

	if (!validQueueFound)
	{
		printf("Failed to find a Vulkan graphics queue family!\n");
		return false;
	}

	// 4. create logical device
	float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
	{
		printf("Failed to create Vulkan logical device!\n");
		return false;
	}

	vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);

	// 5. create command pool
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		printf("Failed to create Vulkan command pool!\n");
		return false;
	}

	// hook into Horus UI Services
	services.setViewport = setViewport;
	services.clearBackbuffer = clearBackbuffer;
	services.draw = draw;
	services.getGfxApiName = []() -> const char* { return "Vulkan"; };

	printf("Vulkan Backend initialized successfully.\n");
	return true;
}

void shutdownVulkan(Services& services)
{
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

}
