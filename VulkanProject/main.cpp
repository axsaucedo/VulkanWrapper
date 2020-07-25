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

	float angle = 0.0f;
	float deltaTime = 0.0f;
	float lastTime = 0.0f;

	std::cout << "Running" << std::endl;
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float now = glfwGetTime();
		deltaTime = now - lastTime;
		lastTime = now;

		angle += 10.f * deltaTime;
		if (angle > 360.0f) {
			angle -= 360.0f;
		}
		vulkanRenderer.updateModel(glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f)));

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