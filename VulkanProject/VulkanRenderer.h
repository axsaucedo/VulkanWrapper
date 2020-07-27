#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <set>
#include <array>
#include <algorithm>

#include "stb_image.h"

#include "Mesh.h"
#include "MeshModel.h"
#include "Utilities.h"

class VulkanRenderer 
{
public:
	VulkanRenderer();

	int init(GLFWwindow* newWindow);

	void updateModel(int modelId, glm::mat4 newModel);

	void cleanup();
	void draw();

	~VulkanRenderer();

private:
	GLFWwindow* window;

	int currentFrame = 0;

	// Scene objects
	std::vector<Mesh> meshList;

	// Scene Settings
	struct UboViewProjection {
		glm::mat4 projection;
		glm::mat4 view;
	} uboViewProjection;

	// Vulkan Components
	// - Main
	VkInstance instance;
	struct {
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} mainDevice;
	VkQueue graphicsQueue;
	VkQueue presentationQueue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;

	std::vector<SwapchainImage> swapchainImages;
	std::vector<VkFramebuffer> swapchainFramebuffers;
	std::vector<VkCommandBuffer> commandBuffers;

	VkImage depthBufferImage;
	VkDeviceMemory depthBufferImageMemory;
	VkImageView depthBufferImageView;

	VkSampler textureSampler;

	// - Descriptors
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout samplerSetLayout;
	VkPushConstantRange pushConstantRange;

	VkDescriptorPool descriptorPool;
	VkDescriptorPool samplerDescriptorPool;
	std::vector<VkDescriptorSet> descriptorSets; // One per swapchain image
	std::vector<VkDescriptorSet> samplerDescriptorSets; // One per texture

	std::vector<VkBuffer> vpUniformBuffer;
	std::vector<VkDeviceMemory> vpUniformBufferMemory;

	// NO LONGER USED BELOW BUT KEEPING FOR REFERENCE, AS THAT'S HOW MODEL WAS DONE VIA DYNAMIC BUFFERS
	//std::vector<VkBuffer> modelDynamicUniformBuffer;
	//std::vector<VkDeviceMemory> modelDynamicUniformBufferMemory;
	//VkDeviceSize minUniformBufferOffset;
	//size_t modelUniformAlignment;
	//Model* modelTransferSpace;

	// - Assets
	std::vector<MeshModel> modelList;

	std::vector<VkImage> textureImages;
	std::vector<VkDeviceMemory> textureImageMemory;
	std::vector<VkImageView> textureImageViews;

	// - Pipeline
	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;

	// - Pools
	VkCommandPool graphicsCommandPool;

	// - Utility
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;

	// - Synchronisation
	std::vector<VkSemaphore> imageAvailable;
	std::vector<VkSemaphore> renderFinished;
	std::vector<VkFence> drawFences;

	// Vulkan Functions
	// - Creation functions
	void createInstance();
	void createLogicalDevice();
	void createSurface();
	void createSwapchain();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createPushConstantRange();
	void createGraphicsPipeline();
	void createDepthBufferImage();
	void createFramebuffers();
	void createCommandPool();
	void createCommandBuffers();
	void createSynchronization();
	void createTextureSampler();

	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();

	void updateUniformBuffers(uint32_t imageIndex);

	// - Record Functions
	void recordCommands(uint32_t currentImage);

	// - Get Functions
	void getPhysicalDevice();

	//// NO LONGER USED BELOW BUT KEEPING FOR REFERENCE, AS THAT'S HOW MODEL WAS DONE VIA DYNAMIC BUFFERS
	//// - Allocate Functions
	//void allocateDynamicBufferTransferSpace();

	// - Getters
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice physicalDevice);
	SwapchainDetails getSwapchainDetails(VkPhysicalDevice device);

	// - Utility functions
	bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
	bool checkDeviceSuitable(VkPhysicalDevice physicalDevice);

	// - Choose functions
	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
	VkFormat chooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

	// - Create functions
	VkImage createImage(uint32_t width, uint32_t height, VkFormat format, 
		VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags,
		VkDeviceMemory* imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkShaderModule createShaderModule(const std::vector<char>& code);

	int createTextureImage(std::string fileName);
	int createTexture(std::string fileName);
	int createTextureDescriptor(VkImageView textureImage);

	void createMeshModel(std::string modelFile);

	// - Loader functions
	stbi_uc* loadTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize);
};

