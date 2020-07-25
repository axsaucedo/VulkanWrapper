#pragma once

#include <fstream>

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

const int MAX_FRAME_DRAWS = 2;
const int MAX_OBJECTS = 2;

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct Vertex
{
	glm::vec3 pos; // Vertex position (x, y, z)
	glm::vec3 col; // Vertex colour (r, g, b)
};

// Indices (locations) of queue families (if they exist at all)
struct QueueFamilyIndices {
	int graphicsFamily = -1; // Location of Graphics Queue Family
	int presentationFamily = -1; // Location of presentation queue family

	// Check if queue families are valid
	bool isValid() {
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};

struct SwapchainDetails {
	VkSurfaceCapabilitiesKHR surfaceCapabilities; // Surface properties, e.g. image size/extent
	std::vector<VkSurfaceFormatKHR> formats; // Surface image formats e.g. RGBA and size of each color
	std::vector<VkPresentModeKHR> presentationModes; // How images should be presented to screen
};

struct SwapchainImage {
	VkImage image;
	VkImageView imageView;
};

static std::vector<char> readFile(const std::string& filename) {
	// OPen stream from given file
	// std::ios::binary telsl stream to read file as binary
	// std::ios::ate tells stream to start reading from end of file
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

	// Check if file stream successfully opened
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open a file!");
	}

	// Get current read position and use to resize file buffer
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> fileBuffer(fileSize);

	// Move read position (seek to) the start of the file
	file.seekg(0);

	// Read the file data into the buffer (stream "filesize" in total)
	file.read(fileBuffer.data(), fileSize);

	// Close stream
	file.close();

	return fileBuffer;
}

static uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties)
{
	// Get properties of physical device memory
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		// Currently we are checking if we are aligned to the current allowed type
		// ie 1 would be 0001 and if i is 2, then we'd see the 1 shifted into 0100
		// This means that we would be comparing the specific allowed type with the current one
		if ((allowedTypes & (1 << i))  // Index of memory type must match corresponding bit in allowed types
			&& (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) // Desired property flags are exactly the same as properties flag
		{
			// This memory type is valid so return its index
			return i;
		}
	}

	throw std::runtime_error("No memory type index found");
}

static void createBuffer(
		VkPhysicalDevice physicalDevice, 
		VkDevice device, 
		VkDeviceSize bufferSize, 
		VkBufferUsageFlags bufferUsage, 
		VkMemoryPropertyFlags bufferProperties, 
		VkBuffer* buffer, 
		VkDeviceMemory* bufferMemory) {
	// -- Create Vertex buffer
	// Information to create a buffer (doesn't include assigning memory)
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = bufferSize; // Size of buffer (size of 1 vertex * number of vertices)
	bufferInfo.usage = bufferUsage; // Multiple types of buffers possible
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Similar to sawp chain images, can share vertex buffers

	VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, buffer);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a vertex buffer");
	}

	// Get buffer memory requirements
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

	// Allocate memory to buffer
	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memRequirements.size;
	memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits, // Index of memory type on physical device that has required bit flags
		bufferProperties); // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT: CPU can interact with memory. VK_MEMORY_PROERTY_HOST_COHERENT_BIT: allows placement of data straight into buffer after mapping (otherwise would have to specify manually)

	// Allocate memory to vkDeviceMemory
	result = vkAllocateMemory(device, &memoryAllocInfo, nullptr, bufferMemory);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate vertext buffer memory");
	}

	// Bind the buffer into the memory to given vertex buffer
	vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}

static void copyBuffer(
	VkDevice device,
	VkQueue transferQueue,
	VkCommandPool transferCommandPool,
	VkBuffer srcBuffer,
	VkBuffer dstBuffer,
	VkDeviceSize bufferSize)
{
	// Command buffer to hold trasnfer commands
	VkCommandBuffer transferCommandBuffer;

	// Command buffer details
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = transferCommandPool;
	allocInfo.commandBufferCount = 1;

	// Allocate command buffer from pool
	vkAllocateCommandBuffers(device, &allocInfo, &transferCommandBuffer);

	// Info to begin the command buffer recod
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We're only use the comand buffer once, so set i for one time submt

	// Begin recording transfer commands
	vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);

	// Region of data to copy from and to
	VkBufferCopy bufferCopyRegion = {};
	bufferCopyRegion.srcOffset = 0; // Copy everything from the start of the first buffer
	bufferCopyRegion.dstOffset = 0; // Copy everything to the start of the second buffer
	bufferCopyRegion.size = bufferSize;

	// Command to copy src buffer to dst buffer
	vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

	// End commands
	vkEndCommandBuffer(transferCommandBuffer);

	// Queue submission information
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCommandBuffer;
	
	// Submit transfer command 
	// Because we don't have many meshes we can have a simple wait for queue to be done (instead of setting the fences/semaphores)
	vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(transferQueue);

	// Free temporary command buffer back to pool
	vkFreeCommandBuffers(device, transferCommandPool, 1, &transferCommandBuffer);

}