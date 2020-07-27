#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

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
	initWindow("Test Window", 1920, 1080);

	std::cout << "Init renderer" << std::endl;
	if (vulkanRenderer.init(window) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	float angle = 0.0f;
	float deltaTime = 0.0f;
	float lastTime = 0.0f;

	std::cout << "Creating Mesh Model" << std::endl;

	int helicopterId = vulkanRenderer.createMeshModel("Models/Intergalactic_Spaceship-(Wavefront).obj");

	std::cout << "Running game loop" << std::endl;
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float now = glfwGetTime();
		deltaTime = now - lastTime;
		lastTime = now;

		angle += 10.f * deltaTime;
		if (angle > 360.0f) {
			angle -= 360.0f;
		}
		
		glm::mat4 testMat = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
		testMat = glm::rotate(testMat, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
		//testMat = glm::rotate(testMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		vulkanRenderer.updateModel(helicopterId, testMat);

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