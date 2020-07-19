#include "Mesh.h"

Mesh::Mesh()
{
}

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, std::vector<Vertex>* vertices)
{
	this->vertexCount = vertices->size();
	this->physicalDevice = newPhysicalDevice;
	this->device = newDevice;
	this->createVertexBuffer(vertices);
}

int Mesh::getVertexCount()
{
	return this->vertexCount;
}

VkBuffer Mesh::getVertexBuffer()
{
	return this->vertexBuffer;
}

void Mesh::destroyVertexBuffer()
{
	vkDestroyBuffer(this->device, this->vertexBuffer, nullptr);
	vkFreeMemory(this->device, this->vertexBufferMemory, nullptr);
}

Mesh::~Mesh()
{
}

void Mesh::createVertexBuffer(std::vector<Vertex>* vertices)
{
	// -- Create Vertex buffer
	// Information to create a buffer (doesn't include assigning memory)
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(Vertex) * vertices->size(); // Size of buffer (size of 1 vertex * number of vertices)
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; // Multiple types of buffers possible, we want vertex buffer
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Similar to sawp chain images, can share vertex buffers

	VkResult result = vkCreateBuffer(this->device, &bufferInfo, nullptr, &this->vertexBuffer);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a vertex buffer");
	}

	// Get buffer memory requirements
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(this->device, this->vertexBuffer, &memRequirements);

	// Allocate memory to buffer
	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memRequirements.size;
	memoryAllocInfo.memoryTypeIndex = this->findMemoryTypeIndex(memRequirements.memoryTypeBits, // Index of memory type on physical device that has required bit flags
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT: CPU can interact with memory. VK_MEMORY_PROERTY_HOST_COHERENT_BIT: allows placement of data straight into buffer after mapping (otherwise would have to specify manually)

	// Allocate memory to vkDeviceMemory
	result = vkAllocateMemory(this->device, &memoryAllocInfo, nullptr, &this->vertexBufferMemory);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate vertext buffer memory");
	}

	// Bind the buffer into the memory to given vertex buffer
	vkBindBufferMemory(this->device, this->vertexBuffer, this->vertexBufferMemory, 0);

	// -- Map memory to vertex data --
	void* data; // 1. Create pointer to point in normal memory (initialised with null)
	vkMapMemory(this->device, this->vertexBufferMemory, 0, bufferInfo.size, 0, &data); // 2. "Map" the vertex buffer memory to that point
	memcpy(data, vertices->data(), (size_t)bufferInfo.size); // 3. Copy memory from vertices vector to the pointer
	vkUnmapMemory(this->device, this->vertexBufferMemory); // 4. Unmap vertex buffer memory
}

uint32_t Mesh::findMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags properties)
{
	// Get properties of physical device memory
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(this->physicalDevice, &memoryProperties);

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
