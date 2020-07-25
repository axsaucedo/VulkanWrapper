#include "Mesh.h"


Mesh::Mesh()
{
}

Mesh::Mesh(
		VkPhysicalDevice newPhysicalDevice, 
		VkDevice newDevice,
		VkQueue transferQueue,
		VkCommandPool transferCommandPool,
		std::vector<Vertex>* vertices,
		std::vector<uint32_t>* indices)
{
	this->vertexCount = vertices->size();
	this->indexCount = indices->size();
	this->physicalDevice = newPhysicalDevice;
	this->device = newDevice;
	this->createVertexBuffer(transferQueue, transferCommandPool, vertices);
	this->createIndexBuffer(transferQueue, transferCommandPool, indices);

	uboModel.model = glm::mat4(1.0f);
}

void Mesh::setModel(glm::mat4 newModel)
{
	this->uboModel.model = newModel;
}

UboModel Mesh::getModel()
{
	return this->uboModel;
}

int Mesh::getVertexCount()
{
	return this->vertexCount;
}

VkBuffer Mesh::getVertexBuffer()
{
	return this->vertexBuffer;
}

int Mesh::getIndexCount()
{
	return this->indexCount;
}

VkBuffer Mesh::getIndexBuffer()
{
	return this->indexBuffer;
}

void Mesh::destroyBuffers()
{
	vkDestroyBuffer(this->device, this->vertexBuffer, nullptr);
	vkFreeMemory(this->device, this->vertexBufferMemory, nullptr);
	vkDestroyBuffer(this->device, this->indexBuffer, nullptr);
	vkFreeMemory(this->device, this->indexBufferMemory, nullptr);
}

Mesh::~Mesh()
{
}

void Mesh::createVertexBuffer(
		VkQueue transferQueue,
		VkCommandPool transferCommandPool, 
		std::vector<Vertex>* vertices)
{
	// Get size of buffer needed for vertices
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

	// Temporary buffer to "stage" vertex data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// Create staging buffer and allocate memory to it ( this will be temporary just for this function) 
	createBuffer(this->physicalDevice, this->device, bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // this will be the source buffer which will only be used for transfer
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferMemory);
	
	// -- Map memory to vertex data --
	void* data; // 1. Create pointer to point in normal memory (initialised with null)
	vkMapMemory(this->device, stagingBufferMemory, 0, bufferSize, 0, &data); // 2. "Map" the vertex buffer memory to that point
	memcpy(data, vertices->data(), (size_t)bufferSize); // 3. Copy memory from vertices vector to the pointer
	vkUnmapMemory(this->device, stagingBufferMemory); // 4. Unmap vertex buffer memory

	// Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data ( Also vertex buffer)
	createBuffer(this->physicalDevice, this->device, bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, // This will be the destination buffer (from the source above) but also is a vertex buffer
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // Memory device_local is on the GPU and only accessible by it and not CPU (host)
		&this->vertexBuffer, &this->vertexBufferMemory);

	// Copy staging buffer into vertex buffer on GPU
	copyBuffer(this->device, transferQueue, transferCommandPool, stagingBuffer, this->vertexBuffer, bufferSize);

	// Clean up staging buffer parts
	vkDestroyBuffer(this->device, stagingBuffer, nullptr);
	vkFreeMemory(this->device, stagingBufferMemory, nullptr);
}

void Mesh::createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices)
{
	// Get size of buffer needed for indeices
	VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();

	// Temporary buffer to "stage" index data before transfering to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(this->physicalDevice, this->device, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer,
		&stagingBufferMemory);

	// Map memory to index buffer
	void* data;
	vkMapMemory(this->device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices->data(), (size_t)bufferSize);
	vkUnmapMemory(this->device, stagingBufferMemory);

	// Create buffer for INDEX data on GPU access only area
	createBuffer(this->physicalDevice, this->device, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		&this->indexBuffer, &this->indexBufferMemory);

	// Copy from staging buffer to GPU access buffer
	copyBuffer(this->device, transferQueue, transferCommandPool, stagingBuffer, this->indexBuffer, bufferSize);
}
