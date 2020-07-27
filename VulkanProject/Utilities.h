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
	glm::vec2 tex; // Texture coords (u, v)
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

static VkCommandBuffer beginCommandBuffer(VkDevice device, VkCommandPool commandPool) {
	// Command buffer to hold trasnfer commands
	VkCommandBuffer commandBuffer;

	// Command buffer details
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	// Allocate command buffer from pool
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	// Info to begin the command buffer recod
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We're only use the comand buffer once, so set i for one time submt

	// Begin recording transfer commands
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

static void endAndSubmitCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer) {

	// End commands
	vkEndCommandBuffer(commandBuffer);

	// Queue submission information
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// Submit transfer command 
	// Because we don't have many meshes we can have a simple wait for queue to be done (instead of setting the fences/semaphores)
	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	// Free temporary command buffer back to pool
	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

}

static void copyBuffer(
	VkDevice device,
	VkQueue transferQueue,
	VkCommandPool transferCommandPool,
	VkBuffer srcBuffer,
	VkBuffer dstBuffer,
	VkDeviceSize bufferSize)
{
	// Create buffer
	VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device, transferCommandPool);

	// Region of data to copy from and to
	VkBufferCopy bufferCopyRegion = {};
	bufferCopyRegion.srcOffset = 0; // Copy everything from the start of the first buffer
	bufferCopyRegion.dstOffset = 0; // Copy everything to the start of the second buffer
	bufferCopyRegion.size = bufferSize;

	// Command to copy src buffer to dst buffer
	vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

	// End and submit buffer to dst buffer
	endAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
}

static void copyImageBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
	VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height) {

	// Create buffer
	VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device, transferCommandPool);

	VkBufferImageCopy imageRegion = {};
	imageRegion.bufferOffset = 0; // Offset into data
	imageRegion.bufferRowLength = 0; // Row length of data to calculate data spacing
	imageRegion.bufferImageHeight = 0; // Image height to calculate data spacing 
	imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Which aspect of image to copy
	imageRegion.imageSubresource.mipLevel = 0; // Mipmap level to copy
	imageRegion.imageSubresource.baseArrayLayer = 0; // Starting array layer if there is array
	imageRegion.imageSubresource.layerCount = 1; // Number of layers to start copy starting at baseArrayLayer
	imageRegion.imageOffset = { 0, 0, 0 }; // Offset into image as opposed to raw data in bufferOffset
	imageRegion.imageExtent = { width, height, 1 };

	// Copy buffer to given image
	vkCmdCopyBufferToImage(transferCommandBuffer, srcBuffer, dstImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);

	// End and submit buffer to dst buffer
	endAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
}

static void transitionImageLayout(VkDevice device, VkQueue queue,
	VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {

	// Create buffer
	VkCommandBuffer commandBuffer = beginCommandBuffer(device, commandPool);

	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.oldLayout = oldLayout; // Layout to transition from
	imageMemoryBarrier.newLayout = newLayout; // Layout to transition to
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // Queue family to trans
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // ASpect of image being altered
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0; // Firsst mip level to start alteations on
	imageMemoryBarrier.subresourceRange.levelCount = 1; // Number of mip levels to start from base
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0; // First layer to start alterations on
	imageMemoryBarrier.subresourceRange.layerCount = 1; // Number of layers to alter starting from baseArrayLayer

	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;

	// If transitioning from new image to image ready to receive data do the following if statement
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = 0; // Memory stage transition must happen after this stage (currently set to 0 meaning it doens't matter)
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // THis is the access mask that we want to ensure the transition happens before 

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	// If transitioning from transfer detination to shader readable
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		srcStage, dstStage, // pipeline stages (match to src and dst access masks just above)
		0, // Dependency flags
		0, nullptr, // Memory barrier count  + data
		0, nullptr, // Buffer memory barrier count + data
		1, &imageMemoryBarrier // Image memory barrier count + data
	);

	// End and submit buffer to dst buffer
	endAndSubmitCommandBuffer(device, commandPool, queue, commandBuffer);
}
