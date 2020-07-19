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
	// Get size of buffer needed for vertices
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

	// Create buffer and allocate memory to it
	createBuffer(this->physicalDevice, this->device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&this->vertexBuffer, &this->vertexBufferMemory);
	
	// -- Map memory to vertex data --
	void* data; // 1. Create pointer to point in normal memory (initialised with null)
	vkMapMemory(this->device, this->vertexBufferMemory, 0, bufferSize, 0, &data); // 2. "Map" the vertex buffer memory to that point
	memcpy(data, vertices->data(), (size_t)bufferSize); // 3. Copy memory from vertices vector to the pointer
	vkUnmapMemory(this->device, this->vertexBufferMemory); // 4. Unmap vertex buffer memory
}
