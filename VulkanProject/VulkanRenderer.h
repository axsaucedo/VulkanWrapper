#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>

class VulkanRenderer 
{
public:
	VulkanRenderer();

	int init(GLFWwindow* newWindow);
	void cleanup();

	~VulkanRenderer();

private:
	GLFWwindow* window;

	// Vulkan Components
	VkInstance instance;
	struct {
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} mainDevice;

	// Vulkan Functions
	// - Creation functions
	void createInstance();

	// - Get Functions
	void getPhysicalDevice();

	// - Utility functions
	bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
};

