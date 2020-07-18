#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>

#include "VulkanRenderer.h"

GLFWwindow* window;
VulkanRenderer vulkanRenderer;

void initWindow(std::string wName = "Test Window", const int width = 800, const int height = 600)
{
	glfwInit();

	// Set GLFW to NOT work with OpenGL
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
}

int main() 
{
	std::cout << "Init window" << std::endl;
	initWindow("Test Window", 800, 600);

	std::cout << "Init renderer" << std::endl;
	if (vulkanRenderer.init(window) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	std::cout << "Running" << std::endl;
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		vulkanRenderer.draw();
	}

	std::cout << "Cleaning up" << std::endl;
	vulkanRenderer.cleanup();

	std::cout << "Destroying window" << std::endl;
	glfwDestroyWindow(window);
	std::cout << "Terminating" << std::endl;
	glfwTerminate();
	
	std::cout << "Success" << std::endl;
	return 0;
}