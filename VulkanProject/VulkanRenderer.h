#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <set>

#include "Utilities.h"

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
	VkQueue graphicsQueue;
	VkQueue presentationQueue;
	VkSurfaceKHR surface;

	// Vulkan Functions
	// - Creation functions
	void createInstance();
	void createLogicalDevice();
	void createSurface();

	// - Get Functions
	void getPhysicalDevice();

	// - Getters
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice physicalDevice);

	// - Utility functions
	bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
	bool checkDeviceSuitable(VkPhysicalDevice physicalDevice);
};

