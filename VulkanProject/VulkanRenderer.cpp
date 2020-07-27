#include "VulkanRenderer.h"

VulkanRenderer::VulkanRenderer()
{
}

int VulkanRenderer::init(GLFWwindow* newWindow)
{
	this->window = newWindow;
	
	try {
		std::cout << "Creating instance" << std::endl;
		this->createInstance();
		std::cout << "Creating surface" << std::endl;
		this->createSurface();
		std::cout << "Creating physical device" << std::endl;
		this->getPhysicalDevice();
		std::cout << "Creating logical device" << std::endl;
		this->createLogicalDevice();
		std::cout << "Creating swapchain" << std::endl;
		this->createSwapchain();
		std::cout << "Creating render pass" << std::endl;
		this->createRenderPass();
		std::cout << "Creating descriptor set layout" << std::endl;
		this->createDescriptorSetLayout();
		std::cout << "Creating push constant range" << std::endl;
		this->createPushConstantRange();
		std::cout << "Creating graphics pipeline" << std::endl;
		this->createGraphicsPipeline();
		std::cout << "Creating depth buffer image" << std::endl;
		this->createDepthBufferImage();
		std::cout << "Creating framebuffers" << std::endl;
		this->createFramebuffers();
		std::cout << "Creating command pool" << std::endl;
		this->createCommandPool();
		std::cout << "Creating command buffers" << std::endl;
		this->createCommandBuffers();
		std::cout << "Creating texture sampler" << std::endl;
		this->createTextureSampler();
		//// NO LONGER USED BELOW BUT KEEPING FOR REFERENCE, AS THAT'S HOW MODEL WAS DONE VIA DYNAMIC BUFFERS
		//std::cout << "Allocating dynamic buffer transfer space" << std::endl;
		//this->allocateDynamicBufferTransferSpace();
		std::cout << "Creating uniform buffers" << std::endl;
		this->createUniformBuffers();
		std::cout << "Creating descriptor pool" << std::endl;
		this->createDescriptorPool();
		std::cout << "Creating descriptor set" << std::endl;
		this->createDescriptorSets();
		std::cout << "Creating synchronisation" << std::endl;
		this->createSynchronization();

		this->uboViewProjection.projection = glm::perspective(
			glm::radians(45.0f),
			(float)this->swapchainExtent.width / (float)this->swapchainExtent.height,
			0.1f, 100.0f);
		this->uboViewProjection.view = glm::lookAt(
			glm::vec3(2.0f, 0.0f, -1.0f), // Where the camara is
			glm::vec3(0.0f, 0.0f, -4.0f), // The target / centre (set to origin here)
			glm::vec3(0.0f, 1.0f, 0.0f)); // The nagle of the camera, ie where up value is - in this case up

		// Vulkan inverts the coordinates, so we need to invert the y coordinate
		uboViewProjection.projection[1][1] *= -1;

		// -- Create a mesh --
		// Vertex Data
		std::vector<Vertex> meshVertices = {
			{{-0.4, 0.4, 0.0}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}, // 0
			{{-0.4, -0.4, 0.0}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 1
			{{0.4, -0.4, 0.0}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, // 2
			{{0.4, 0.4, 0.0}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 3
		};
		std::vector<Vertex> meshVertices2 = {
			{{-0.25, 0.6, 0.0}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}, // 0
			{{-0.25, -0.6, 0.0}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 1
			{{0.25, -0.6, 0.0}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, // 2
			{{0.25, 0.6, 0.0}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}, // 3
		};
		// Index Data
		std::vector<uint32_t> meshIndices = {
			0, 1, 2,
			2, 3, 0
		};
		// Mesh
		Mesh firstMesh = Mesh(
			this->mainDevice.physicalDevice,
			this->mainDevice.logicalDevice,
			this->graphicsQueue,
			this->graphicsCommandPool,
			&meshVertices,
			&meshIndices,
			this->createTexture("marble006-color.jpg"));
		Mesh secondMesh = Mesh(
			this->mainDevice.physicalDevice,
			this->mainDevice.logicalDevice,
			this->graphicsQueue,
			this->graphicsCommandPool,
			&meshVertices2,
			&meshIndices,
			this->createTexture("pavingstones070-color.jpg"));

		this->meshList.push_back(firstMesh);
		this->meshList.push_back(secondMesh);
	}
	catch (const std::runtime_error& e) {
		std::cout << "ERROR" << e.what() << std::endl;
		printf("Error: %s\n", e.what());
		return EXIT_FAILURE;
	}

	std::cout << "Success init" << std::endl;
	return 0;
}

void VulkanRenderer::updateModel(int modelId, glm::mat4 newModel)
{
	if (modelId >= this->meshList.size()) return;

	this->meshList[modelId].setModel(newModel);
}

void VulkanRenderer::cleanup()
{
	// Wait until no actions are being run until destroying
	vkDeviceWaitIdle(this->mainDevice.logicalDevice);

	// NO LONGER USED BELOW BUT KEEPING FOR REFERENCE, AS THAT'S HOW MODEL WAS DONE VIA DYNAMIC BUFFERS
	//_aligned_free(this->modelTransferSpace);

	vkDestroyDescriptorPool(this->mainDevice.logicalDevice, this->samplerDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(this->mainDevice.logicalDevice, this->descriptorSetLayout, nullptr);

	vkDestroySampler(this->mainDevice.logicalDevice, this->textureSampler, nullptr);

	for (size_t i = 0; i < this->textureImages.size(); i++) {
		vkDestroyImageView(this->mainDevice.logicalDevice, this->textureImageViews[i], nullptr);
		vkDestroyImage(this->mainDevice.logicalDevice, this->textureImages[i], nullptr);
		vkFreeMemory(this->mainDevice.logicalDevice, this->textureImageMemory[i], nullptr);
	} 

	vkDestroyImageView(this->mainDevice.logicalDevice, this->depthBufferImageView, nullptr);
	vkDestroyImage(this->mainDevice.logicalDevice, this->depthBufferImage, nullptr);
	vkFreeMemory(this->mainDevice.logicalDevice, this->depthBufferImageMemory, nullptr);

	// NO LONGER USED BELOW BUT KEEPING FOR REFERENCE, AS THAT'S HOW MODEL WAS DONE VIA DYNAMIC BUFFERS
	vkDestroyDescriptorPool(this->mainDevice.logicalDevice, this->descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(this->mainDevice.logicalDevice, this->descriptorSetLayout, nullptr);

	for (size_t i = 0; i < this->swapchainImages.size(); i++) {
		vkDestroyBuffer(this->mainDevice.logicalDevice, this->vpUniformBuffer[i], nullptr);
		vkFreeMemory(this->mainDevice.logicalDevice, this->vpUniformBufferMemory[i], nullptr);
		//// NO LONGER USED BELOW BUT KEEPING FOR REFERENCE, AS THAT'S HOW MODEL WAS DONE VIA DYNAMIC BUFFERS
		//vkDestroyBuffer(this->mainDevice.logicalDevice, this->modelDynamicUniformBuffer[i], nullptr);
		//vkFreeMemory(this->mainDevice.logicalDevice, this->modelDynamicUniformBufferMemory[i], nullptr);
	}

	for (size_t i = 0; i < this->meshList.size(); i++) {
		this->meshList[i].destroyBuffers();
	}

	for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
	{
		vkDestroySemaphore(this->mainDevice.logicalDevice, this->renderFinished[i], nullptr);
		vkDestroySemaphore(this->mainDevice.logicalDevice, this->imageAvailable[i], nullptr);
		vkDestroyFence(this->mainDevice.logicalDevice, this->drawFences[i], nullptr);
	}
	vkDestroyCommandPool(this->mainDevice.logicalDevice, this->graphicsCommandPool, nullptr);
	for (auto framebuffer : this->swapchainFramebuffers) {
		vkDestroyFramebuffer(this->mainDevice.logicalDevice, framebuffer, nullptr);
	}
	vkDestroyPipeline(this->mainDevice.logicalDevice, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(this->mainDevice.logicalDevice, pipelineLayout, nullptr);
	vkDestroyRenderPass(this->mainDevice.logicalDevice, renderPass, nullptr);
	for (auto image : swapchainImages) {
		vkDestroyImageView(this->mainDevice.logicalDevice, image.imageView, nullptr);
	} 
	vkDestroySwapchainKHR(this->mainDevice.logicalDevice, swapchain, nullptr);
	vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
	vkDestroyDevice(this->mainDevice.logicalDevice, nullptr);
	vkDestroyInstance(instance, nullptr);
}

void VulkanRenderer::draw()
{
	// This function will do the following
	// 0. Wait until fence (lock) can be acquired to avoid adding too many items into the queue
	// 1. Get next available image to draw to and set something to signal when we're finished with the image (a semafore)
	// 2. Submit command buffer to queue for execution, making sure it waits for the image to signaled as available before drawing
	// and signals when it has finished rendering
	// 3. Present image to screen when it has signalled finished rendering 

	// 0. Wait for lock
    // Wait until the Fence is actually available in order to go further
	vkWaitForFences(this->mainDevice.logicalDevice, 1, &this->drawFences[this->currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	// Manually reset (close) fences
	vkResetFences(this->mainDevice.logicalDevice, 1, &this->drawFences[this->currentFrame]);

	// -- 1. Get next image --
	uint32_t imageIndex;
	// Get index of next image to be drawn to and signal semaphore when ready to be drawn to
	vkAcquireNextImageKHR(this->mainDevice.logicalDevice, this->swapchain, std::numeric_limits<uint64_t>::max(), this->imageAvailable[this->currentFrame], VK_NULL_HANDLE, &imageIndex);

	this->recordCommands(imageIndex);
	this->updateUniformBuffers(imageIndex);

	// -- 2. Submit command buffer to render --
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1; // Number of semaphores to wait on
	submitInfo.pWaitSemaphores = &this->imageAvailable[this->currentFrame]; // List of semaphores to wait on
	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};
	submitInfo.pWaitDstStageMask = waitStages; // Stages to check semaphores at
	submitInfo.commandBufferCount = 1; // Number of command buffers to submit
	submitInfo.pCommandBuffers = &this->commandBuffers[imageIndex]; // Command buffer to submit
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &this->renderFinished[this->currentFrame];

	// Submit the command buffer selected (by imageIndex index) into the queue provided, which is the graphicsQueue
	// and acquire the fence lock to make sure no other images start processing
	VkResult result = vkQueueSubmit(this->graphicsQueue, 1, &submitInfo, this->drawFences[currentFrame]);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit command buffer to queue");
	}

	// -- 3. Present rendered image to screen --
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1; // Number of semaphores to wait on
	presentInfo.pWaitSemaphores = &this->renderFinished[this->currentFrame]; // Semaphores to wait on
	presentInfo.swapchainCount = 1; // Number of swapchains to present to
	presentInfo.pSwapchains = &swapchain; // Swapchain to present images to
	presentInfo.pImageIndices = &imageIndex; // Index of images in swapchains to present

	result = vkQueuePresentKHR(this->presentationQueue, &presentInfo);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present image");
	}

	// Get next fame keeping value below max number of frame draws
	this->currentFrame = (this->currentFrame + 1) % MAX_FRAME_DRAWS;
}

VulkanRenderer::~VulkanRenderer()
{
}

void VulkanRenderer::createInstance()
{
	// Information about the app we're building (not the vulkan app specifically)
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan App"; // Custom name of the app we're building 
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	std::vector<const char*> instanceExtensions = std::vector<const char*>();

	// Set up instance extensions instance will use
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	// Get thes exact Vulkan extensions that GLFW requires to talk to vulkan to build windows
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (size_t i = 0; i < glfwExtensionCount; i++) {
		instanceExtensions.push_back(glfwExtensions[i]);
	}

	if (!checkInstanceExtensionSupport(&instanceExtensions)) {
		throw std::runtime_error("VKInstance does not support instance extensions");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	// TODO: Setup Validation Layers that the instance will use
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;

	// Create Vulkan instance
	VkResult result = vkCreateInstance(&createInfo, nullptr, &this->instance);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vulkan Instance.");
	}
}

void VulkanRenderer::createLogicalDevice()
{
	// Get the queue family indices for the chosen physical device
	QueueFamilyIndices indices = this->getQueueFamilies(mainDevice.physicalDevice);

	// Vector for queue creation information, and set for family indices to ensure we don't do creation twice if same queue index
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily };

	// The queue of the logical device that needs ot be created as well as information required to do so
	for (int queueFamilyIndex : queueFamilyIndices) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex; // The index of the family to create a queue from
		queueCreateInfo.queueCount = 1; // Numbers of queues to create
		float priority = 1.0f;
		queueCreateInfo.pQueuePriorities = &priority; // Vulkan neeeds to know how to handle multiple queues, so decide priority

		queueCreateInfos.push_back(queueCreateInfo);
	}

	// Information to create logical device (sometimes called "device")
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()); // Number of queue create infos
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data(); // List of queue create infos so device can create required queues
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()); // number of enabled logical device extensions (no needd as its logical)
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

	// Physical device features the logical device will be using
	// By default features will be false 
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE; // Enabling anisotropy

	deviceCreateInfo.pEnabledFeatures = &deviceFeatures; // Phyiscal device features logical device will use

	// Create the logical device for the physical device
	VkResult result = vkCreateDevice(this->mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &this->mainDevice.logicalDevice);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a logical device!");
	}

	// Queues are created at the same time as the device...
	// So we want to handle to the queues
	// From given logical device, of given queue family, of given queue index (0 since only queue), place reference in given queue
	vkGetDeviceQueue(this->mainDevice.logicalDevice, indices.graphicsFamily, 0, &this->graphicsQueue);
	vkGetDeviceQueue(this->mainDevice.logicalDevice, indices.presentationFamily, 0, &this->presentationQueue);
}

void VulkanRenderer::createSurface()
{
	// Creating a surface createInfo Struct, runs create surface function, returns result
	VkResult result = glfwCreateWindowSurface(this->instance, this->window, nullptr, &this->surface);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create surface");
	}
}

void VulkanRenderer::createSwapchain()
{
	// Get Swap chain details so we can pick the best settings
	SwapchainDetails swapchainDetails = getSwapchainDetails(this->mainDevice.physicalDevice);

	// 1. CHOOSE BEST SURFACE FORMAT
	VkSurfaceFormatKHR surfaceFormat = this->chooseBestSurfaceFormat(swapchainDetails.formats);

	// 2. CHOOSE BEST PRESENTATION MODE
	VkPresentModeKHR presentMode = this->chooseBestPresentationMode(swapchainDetails.presentationModes);

	// 3. CHOOSE EBST SWAP CHAIN IMAGE RESOLUTION
	VkExtent2D extent = this->chooseSwapExtent(swapchainDetails.surfaceCapabilities);

	// How many images are in the swap chain ? Get one more than mmin to allow tripple buffering
	uint32_t imageCount = swapchainDetails.surfaceCapabilities.minImageCount + 1;

	// If image count higher than the max then clamp down to max as otherwise it would exceed limit
	// If it is 0  then it is limitless
	if (swapchainDetails.surfaceCapabilities.maxImageCount > 0
			&& swapchainDetails.surfaceCapabilities.maxImageCount < imageCount) {
		imageCount = swapchainDetails.surfaceCapabilities.maxImageCount;
	}

	// Creation info for swapchain
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = this->surface;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.imageExtent = extent;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageArrayLayers = 1; // Number of layers for each image in chain
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; 
	swapchainCreateInfo.preTransform = swapchainDetails.surfaceCapabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.clipped = VK_TRUE; // Whether to clip part of images not on screen (eg behind another window off screen)

	// Get queue family indices
	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

	// If graphics and presentation families are different, then swapchain must let images be shared between families
	if (indices.graphicsFamily != indices.presentationFamily)
	{
		// Queues to share between
		uint32_t queueFamilyIndices[] = {
			(uint32_t)indices.graphicsFamily,
			(uint32_t)indices.presentationFamily
		};

		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Image share handling
		swapchainCreateInfo.queueFamilyIndexCount = 2; // Number of queues to share images between
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices; // Array of queues to share between
	}
	else {
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	// If old swap chain been destroyed and this one replaces it, then link old one to quickly hand over responsibilities
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(this->mainDevice.logicalDevice, &swapchainCreateInfo, nullptr, &this->swapchain);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create swap function");
	}

	// Storing for later reference
	this->swapchainImageFormat = surfaceFormat.format;
	this->swapchainExtent = extent;

	uint32_t swapchainImageCount;
	vkGetSwapchainImagesKHR(this->mainDevice.logicalDevice, this->swapchain, &swapchainImageCount, nullptr);

	std::vector<VkImage> images(swapchainImageCount);
	vkGetSwapchainImagesKHR(this->mainDevice.logicalDevice, this->swapchain, &swapchainImageCount, images.data());
	
	for (VkImage image : images) {
		// store full value of image explicitly
		SwapchainImage swapchainImage = {};
		swapchainImage.image = image;
		swapchainImage.imageView = this->createImageView(image, swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

		swapchainImages.push_back(swapchainImage);
	}
}

void VulkanRenderer::createRenderPass()
{
	// - ATTACHMENTS
	// Colour attachment of render pass
	VkAttachmentDescription colourAttachment = {};
	colourAttachment.format = this->swapchainImageFormat; // Format to use for attachment
	colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // Number of samples to write for multisampling
	colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Describes what to do with attachment before rendering
	colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Describes what to do with attachment after rendering
	colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // What to do with stencil before rendering
	colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // What to do with stencil after rendering

	// Framebuffer data will be stored as an image, but images can be given different data layouts
	// to give optimal use for certain operations
	colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Image data layout before render pass starts
	colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Image data layout after render pass (to change to)

	// DEpth attachment of render pass
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = this->chooseSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// - REFERENCES
	// Attachment reference uses an attachment indext that refers to in dex in the atachment list passed to renderpasscreateinfo
	VkAttachmentReference colourAttachmentReference = {};
	colourAttachmentReference.attachment = 0;
	colourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Depth attachment reference
	VkAttachmentReference depthAttachmentReference = {};
	depthAttachmentReference.attachment = 1; // Index array like definition of what position attachment is (one different to above)
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Summary of colourattachment and colouattachemntrefrence:
	// * We expect the initial layout would be layout-UNDEFINED
	// * Then when starting the subpass, we will convert it to the attachmentReference above (ATTACHMENT_OPTIMAL)
	// * Finally the image will be convertedd back into the finalLayout PRESENT_SRC_KHR

	// Information about a particualr subpass the render pass is using
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Pipeline type subpass is to be bound to
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colourAttachmentReference;
	subpass.pDepthStencilAttachment = &depthAttachmentReference;

	// Need to determine when layout transitions occur using subpass dependencies (which also create implicit layout transitions)
	// The reason why it needs to be defined is because this is parallel processing
	std::array<VkSubpassDependency, 2> subpassDependencies;

	// Conversion from VK_IMAGE_LAYOUR_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMEENT_OPTIMAL
	subpassDependencies[0].dependencyFlags = 0;
	// ~> transition must happen after the following...
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL; // This is where we're coming from, subpass_external is a keyword that specifies everything that takes place outside subpasses
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // Which staage of the pipeline needs to happen first - This value means the end of the pipeline 
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT; // This means the memory opeation that needs to happen before you can do the conversaion. Read bi tis whn you are reading / presenting to the screen. It needs to be read from before we can convert to optimal
	// ~> but transition must happen before the following...
	subpassDependencies[0].dstSubpass = 0; // THe index of the subpass that it's refering to, in this case we only have one
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Transition from undefined to colour_optimal has to happend before the srcStageMask and before the dstStageMask
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Conversion to optimal has to happen before we get to the colour output and before it attempts to read and write to it.

	// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMEENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	subpassDependencies[1].dependencyFlags = 0;
	// ~> transition must happen after the following...
	subpassDependencies[1].srcSubpass = 0; // It has to happen after our main subpass has done its draw operation
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Basically specifies it has to happen after the dstSubpass of the previously defined rules in the subpass denednecy above
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Same as per the dstAccessMas of the subpass dependency above
	// ~> but transition must happen before the following...
	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL; // Has to happen before "exiting" to outside 
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // Must happen before the start of the first subpass
	subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT; // Same as previous but with dst mask

	std::array<VkAttachmentDescription, 2> renderPassAttachments = { colourAttachment, depthAttachment };

	///  Create infor for render pass
	VkRenderPassCreateInfo renderPassCreateInfo = { };
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
	renderPassCreateInfo.pAttachments = renderPassAttachments.data();
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassCreateInfo.pDependencies = subpassDependencies.data();

	VkResult result = vkCreateRenderPass(this->mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &this->renderPass);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass");
	}
}

void VulkanRenderer::createDescriptorSetLayout()
{
	// - Uniform values descriptor set layoutc
	// MVP Binding info
	VkDescriptorSetLayoutBinding vpLayoutBinding = {};
	vpLayoutBinding.binding = 0; // this is linked to the vertex chader as per (binding = 0)
	vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Type of descriptor (uniform, dynamic uniform, image sampler for textures, etc)
	vpLayoutBinding.descriptorCount = 1;
	vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Shader stage to bind to
	vpLayoutBinding.pImmutableSamplers = nullptr; // For textures: can make sampler data immutable

	//// NO LONGER USED BELOW BUT KEEPING FOR REFERENCE, AS THAT'S HOW MODEL WAS DONE VIA DYNAMIC BUFFERS
	//// Model binding info
	//VkDescriptorSetLayoutBinding modelLayoutBinding = {};
	//modelLayoutBinding.binding = 1;
	//modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	//modelLayoutBinding.descriptorCount = 1;
	//modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	//modelLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { vpLayoutBinding };

	// Create descriptor set layout with given bindings
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size()); // Number of binding infos
	layoutCreateInfo.pBindings = layoutBindings.data(); // Array of binding infos

	// Create Descriptor set layout
	VkResult result = vkCreateDescriptorSetLayout(this->mainDevice.logicalDevice, &layoutCreateInfo, nullptr, &this->descriptorSetLayout);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
	
	// - Extra sampler descriptor set layout
	// texture binding info
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo textureLayoutCreateInfo = {};
	textureLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	textureLayoutCreateInfo.bindingCount = 1;
	textureLayoutCreateInfo.pBindings = &samplerLayoutBinding;

	// Create descriptro set layout
	result = vkCreateDescriptorSetLayout(this->mainDevice.logicalDevice, &textureLayoutCreateInfo, nullptr, &this->samplerSetLayout);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}


void VulkanRenderer::createPushConstantRange()
{
	// Define push constant values (no "create" needed)
	this->pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Shader stage push constant will go to
	this->pushConstantRange.offset = 0; // Offset into given data to pass to push constant
	this->pushConstantRange.size = sizeof(Model); // Size of data being passed
}

void VulkanRenderer::createGraphicsPipeline()
{
	std::vector<char> vertexShaderCode = readFile("Shaders/vert.spv");
	std::vector<char> fragmentShaderCode = readFile("Shaders/frag.spv");

	VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
	VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

	// -- SHADER STAGE CREATION INFORMATION --
	// Vertex Stage creation information
	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // Shader stage name
	vertexShaderCreateInfo.module = vertexShaderModule; // Shader module to be used by stage
	vertexShaderCreateInfo.pName = "main"; // Name of entry point function in shader file

	// Fragment stage creation information
	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT; // Shader stage name
	fragmentShaderCreateInfo.module = fragmentShaderModule; // Shader module to be used by stage
	fragmentShaderCreateInfo.pName = "main"; // Name of entry point function in shader file

	// Put shader stage creation info in to array
	// Graphics Pipeline creation info requires array of shader stage creates
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

	// How the data for a single vertex (including such as positino, colour, texture coords, normals, etc) is as a whole
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0; // Can bind multiple streams of data, this defines which one
	bindingDescription.stride = sizeof(Vertex); // Size of a single vertex object
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Here you select whether you want to draw one object at a time (VK_VERTEX_INPUT_RATE_VERTEX) or one of the vertex for each at a time (VK_VERTEX_INPUT_RATE_INSTANCE)

	// How the data for an attribute is defined within a vertex
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;
	// Position Attribute 
	attributeDescriptions[0].binding = 0; // Which binding the data is at (should be same as above - unless you have stream of data)
	attributeDescriptions[0].location = 0; // Location in shader where data will be read from
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // Format the data will take (also helps define the size of the data)
	attributeDescriptions[0].offset = offsetof(Vertex, pos); // Where this attribute is defined in the data for a single vertex
	// Colour Attribute 
	attributeDescriptions[1].binding = 0; 
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; 
	attributeDescriptions[1].offset = offsetof(Vertex, col); 
	// TExture Attribute
	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, tex);

	// CREATE PIPELINE
	// -- Vertex Input --
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1; 
	vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription; // List of Vertex Binding Descriptions (data spacing / stride info)
	vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // List of vertex attr descriptions (data format and where from)

	// -- Input Assembly --
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Primitive type to assemble vertices as
	inputAssembly.primitiveRestartEnable = VK_FALSE; // Allow overriding of strip topology to start new primitives

	// -- Viewport and scissor --
	// Create a viewport info struct
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapchainExtent.width;
	viewport.height = (float)swapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// Create a scissor info struct
	VkRect2D scissor = {};
	scissor.offset = { 0,0 };
	scissor.extent = swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	//// -- Dynamic State --
	//// Dynamic states to enable (disabpled for simplicity)
	//std::vector<VkDynamicState> dynamicStateEnables;
	//dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT); // Dynamic viewport: can resize in command buffer with vkCmdSetViewport(commandbuffer, 0, 1, &viewport);
	//dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR); // Dynamic scissor: can resize in command buffer with vkCmdSetScissor(commandbuffer, 0, 1, &scissor);

	//// Dynamic state creation info
	//VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	//dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	//dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	//dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();

	// -- Rasterizer -- 
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE; // Change if fragments beyond near/far planes are clipped (default) or clamped
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE; // Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL; // How to handle filling points between vertices
	rasterizerCreateInfo.lineWidth = 1.0f; // How thick lines should be drawn
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT; // Which face of the triangle to draw
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // Winding to determine which side is front
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE; // Whether to add depth bias to fragments ( good for stopping "shadow acne")

	// -- Mutlisampling --
	// Form of antialiasing (removing jaggered edges on a shape). This is a subset of supersampling (scaled up version of image, take points around, blend and downsize) - multisampling is similar but does only for edges
	// Tends to be hard to implement, so you need an intermediary image to implement this, and this is what we call the resolve attachment (like color attachment)
	// We are going to disable multisampling for now
	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
	multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingCreateInfo.sampleShadingEnable = VK_FALSE; // Enable multisample shading or not
	multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // Number of samples to use per fragmnt

	// -- Blending --
	// If drawn object and then another on top, if something is transparent, needs to blend to have transparency feel
	// This defines how things would blend together when there is transparency

	// Blend attachment state (how blending is handled)
	VkPipelineColorBlendAttachmentState colourState = {};
	colourState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT; // The colours to apply blending to
	colourState.blendEnable = VK_TRUE; // JUst means that we want to enable blending

	// Blending uses equation: (srcColorBlendFactor * new colour) colourBlendOp (dstColorBlendFactor * old colour)
	colourState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colourState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colourState.colorBlendOp = VK_BLEND_OP_ADD;

	// Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new colour) + (VK_BLEND_FACTOR_ONE_MINUS_ALPHA * old colour)
	// Or more specifically: (new colour alpha * new colour) ((1 - new colour alpha) * old colour)

	// Now we need to blend the alpha values
	// We'll set it to leave the old alpha and use the new one
	colourState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colourState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colourState.alphaBlendOp = VK_BLEND_OP_ADD;
	//Summarized: (1 * new alpha) + (0 * old alpha) = new alpha

	VkPipelineColorBlendStateCreateInfo colourBlendingCreateInfo = { };
	colourBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colourBlendingCreateInfo.logicOpEnable = VK_FALSE; // Alternative to calculations is to use logical operations
	colourBlendingCreateInfo.attachmentCount = 1;
	colourBlendingCreateInfo.pAttachments = &colourState;
	// We wont be using the logic operations, instead we wil be using the attachment (above)
	//colourBlendingCreateInfo.logicOp = VK_LOGIC_OP_COPY; // What is the way that you want to do the operations

	// -- Pipeline layout --
	std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { descriptorSetLayout, samplerSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &this->pushConstantRange;

	// Create pipelinelayout
	VkResult result = vkCreatePipelineLayout(this->mainDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &this->pipelineLayout);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Pipeline layout");
	}

	// -- DEPTH STENCIL TESTING --
	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
	depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCreateInfo.depthTestEnable = VK_TRUE; // Enable checking depth to determine frament write
	depthStencilCreateInfo.depthWriteEnable = VK_TRUE; // Enable writing to depth buffer (to replace old values)
	depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS; // Comparison operation that allows an overwrite (whether is in front)
	depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE; // Depth bounds test: does the depth value exist between two bounds?
	depthStencilCreateInfo.stencilTestEnable = VK_FALSE;

	// -- Graphics Pipeline Creation --
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStages;
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pDynamicState = nullptr;
	pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colourBlendingCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
	pipelineCreateInfo.layout = this->pipelineLayout;
	pipelineCreateInfo.renderPass = this->renderPass;
	pipelineCreateInfo.subpass = 0;

	// Pipeline derivatives:  Can create multiple pipelines that derive from one another for optimisation
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Existing pipeline to derive from...
	pipelineCreateInfo.basePipelineIndex = -1; // or index of pipeline being created to derive from (in case creating multiple pipelines at once)

	// Create graphics pipeline
	result = vkCreateGraphicsPipelines(this->mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &this->graphicsPipeline);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline");
	}

	// Destroy shader modules, no longer needed after pipeline created
	vkDestroyShaderModule(this->mainDevice.logicalDevice, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(this->mainDevice.logicalDevice, vertexShaderModule, nullptr);
}

void VulkanRenderer::createDepthBufferImage()
{
	// Get supported format for deth buffer
	VkFormat depthFormat = this->chooseSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);

	// Create depth buffer image
	this->depthBufferImage = this->createImage(
		this->swapchainExtent.width,
		this->swapchainExtent.height,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&this->depthBufferImageMemory
	);

	// Create depth buffer image view
	this->depthBufferImageView = this->createImageView(this->depthBufferImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void VulkanRenderer::createFramebuffers()
{
	// Resize framebuffer count to equal to swapchain image count
	this->swapchainFramebuffers.resize(swapchainImages.size());

	// Create a framebuffer for eachi swap chain image
	for (size_t i = 0; i < this->swapchainFramebuffers.size(); i++) {
		std::array<VkImageView, 2> attachments = {
			this->swapchainImages[i].imageView,
			this->depthBufferImageView
		};

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = this->renderPass; // render pass layout the framebuffer will be used with
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferCreateInfo.pAttachments = attachments.data(); // List of attachments (1:1 with render pass)
		framebufferCreateInfo.width = this->swapchainExtent.width; 
		framebufferCreateInfo.height = this->swapchainExtent.height; 
		framebufferCreateInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(this->mainDevice.logicalDevice, &framebufferCreateInfo, nullptr, &swapchainFramebuffers[i]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create a framebuffer");
		}
	}
}

void VulkanRenderer::createCommandPool()
{
	// Get indices of queue families from device
	QueueFamilyIndices queueFamilyIndices = this->getQueueFamilies(this->mainDevice.physicalDevice);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // This allows us to re-record the commands in this pool
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily; // Queue family that buffers from this command pool will use

	// Create a graphics queue family command pool
	VkResult result = vkCreateCommandPool(this->mainDevice.logicalDevice, &poolInfo, nullptr, &this->graphicsCommandPool);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool");
	}
}

void VulkanRenderer::createCommandBuffers()
{
	// Resize command buffer count to have one for each framebuffer
	this->commandBuffers.resize(this->swapchainFramebuffers.size());

	// We're not creating memory, we're just allocating it (hence why it's an allocate instead of create)
	VkCommandBufferAllocateInfo cbAllocInfo = {};
	cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbAllocInfo.commandPool = this->graphicsCommandPool;
	// vkCmdExecuteCommands(buffer) <- primary = executed by queue, secondary = executed by primary
	// PRIMARIY: buffer you submit directly to the quuee, can't be called by other buffers
	// SECONDARY: Buffer can't be called directly, can be called from other buffers via "vkCmdExecuteCommands when reading commands
	cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; 
	cbAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	VkResult result =  vkAllocateCommandBuffers(this->mainDevice.logicalDevice, &cbAllocInfo, commandBuffers.data());

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command bfufers");
	}
}

void VulkanRenderer::createSynchronization()
{
	this->imageAvailable.resize(MAX_FRAME_DRAWS);
	this->renderFinished.resize(MAX_FRAME_DRAWS);
	this->drawFences.resize(MAX_FRAME_DRAWS);
	
	// Semaphore creation information
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;

	// Fence creation information
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
	{
		if (vkCreateSemaphore(this->mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &this->imageAvailable[i]) != VK_SUCCESS ||
			vkCreateSemaphore(this->mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &this->renderFinished[i]) != VK_SUCCESS ||
			vkCreateFence(this->mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &this->drawFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create a Semaphore");
		}
	}
}

void VulkanRenderer::createTextureSampler()
{
	// Sample creation info
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR; // How to render when image is magnified on screen
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR; // How to render when image is minified on screen
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // How to handle texture wrap in U (x) direction
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT; // How to handle texture wrap in V (y) direction
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT; // How to handle texture wrap in W (z) direction
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; // Border beyond texture (only works for border clamp)
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE; // Yes to normalised coordinates, between 0 and 1 - whether coords should be normalised between 0 and 1
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; // Mipmap interpolation mode
	samplerCreateInfo.mipLodBias = 0.0f; // Level of detail bias for mip level
	samplerCreateInfo.minLod = 0.0f; // minimum level of detail to pick mip level, but we're not using mipmaps
	samplerCreateInfo.maxLod = 0.0f;
	samplerCreateInfo.anisotropyEnable = VK_TRUE; // Anisotropic is drawing a texture and stretching the borders to mimic eyesight borders (an antialiasing technique)
	samplerCreateInfo.maxAnisotropy = 16; // Amount of samples level being taken for anisotropy

	VkResult result = vkCreateSampler(this->mainDevice.logicalDevice, &samplerCreateInfo, nullptr, &this->textureSampler);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create sampler");
	}
}

void VulkanRenderer::createUniformBuffers()
{
	// ViewProjection buffer size
	VkDeviceSize vpBufferSize = sizeof(UboViewProjection);

	// NO LONGER USED BELOW BUT KEEPING FOR REFERENCE, AS THAT'S HOW MODEL WAS DONE VIA DYNAMIC BUFFERS
	//VkDeviceSize modelBufferSize = this->modelUniformAlignment * MAX_OBJECTS;

	// One uniform buffer for each image (and by extension, command buffer)
	this->vpUniformBuffer.resize(this->swapchainImages.size());
	this->vpUniformBufferMemory.resize(this->swapchainImages.size());
	// NO LONGER USED BELOW BUT KEEPING FOR REFERENCE, AS THAT'S HOW MODEL WAS DONE VIA DYNAMIC BUFFERS
	//this->modelDynamicUniformBuffer.resize(this->swapchainImages.size());
	//this->modelDynamicUniformBufferMemory.resize(this->swapchainImages.size());

	// Create uniform buffers
	for (size_t i = 0; i < this->swapchainImages.size(); i++) {
		createBuffer(
			this->mainDevice.physicalDevice, 
			this->mainDevice.logicalDevice, 
			vpBufferSize, 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&this->vpUniformBuffer[i],
			&this->vpUniformBufferMemory[i]);

		//// NO LONGER USED BELOW BUT KEEPING FOR REFERENCE, AS THAT'S HOW MODEL WAS DONE VIA DYNAMIC BUFFERS
		//createBuffer(
		//	this->mainDevice.physicalDevice,
		//	this->mainDevice.logicalDevice,
		//	modelBufferSize,
		//	VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		//	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		//	&this->modelDynamicUniformBuffer[i],
		//	&this->modelDynamicUniformBufferMemory[i]);
	}
}

void VulkanRenderer::createDescriptorPool()
{
	// - Create Uniform Descriptor Pool
	// Type of descriptors + how many DESCRIPTORS, not descriptor sets (combined makes pool size)
	VkDescriptorPoolSize vpPoolSize = {};
	vpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vpPoolSize.descriptorCount = static_cast<uint32_t>(this->vpUniformBuffer.size());

	//// NO LONGER USED BELOW BUT KEEPING FOR REFERENCE, AS THAT'S HOW MODEL WAS DONE VIA DYNAMIC BUFFERS
	//// Model pool (dynamic)
	//VkDescriptorPoolSize modelPoolSize = {};
	//modelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	//modelPoolSize.descriptorCount = static_cast<uint32_t>(this->modelDynamicUniformBuffer.size());

	// List of pools
	std::vector<VkDescriptorPoolSize> descriptorPoolSizes = { vpPoolSize };

	// Data to create descriptor pool
	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets = static_cast<uint32_t>(this->swapchainImages.size()); // Max number of descriptor sets that can eb creataed from pool
	poolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size()); // Amount of pool sizes being passed
	poolCreateInfo.pPoolSizes = descriptorPoolSizes.data();

	// Create descriptor pool
	VkResult result = vkCreateDescriptorPool(this->mainDevice.logicalDevice, &poolCreateInfo, nullptr, &this->descriptorPool);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool");
	}

	// - Create Sampler Descriptor Pool
	VkDescriptorPoolSize samplerPoolSize = {};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = MAX_OBJECTS;

	VkDescriptorPoolCreateInfo samplerPoolCreateInfo = {};
	samplerPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	samplerPoolCreateInfo.maxSets = MAX_OBJECTS;
	samplerPoolCreateInfo.pPoolSizes = &samplerPoolSize;

	result = vkCreateDescriptorPool(this->mainDevice.logicalDevice, &samplerPoolCreateInfo, nullptr, &this->samplerDescriptorPool);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool");
	}
}

void VulkanRenderer::createDescriptorSets()
{
	// Resize descriptor set list so one for every buffer
	this->descriptorSets.resize(this->swapchainImages.size());

	// We create a list with all the values the same as that's how it references in the allocation
	std::vector<VkDescriptorSetLayout> setLayouts(this->swapchainImages.size(), this->descriptorSetLayout);

	// Descriptor set allocation info
	VkDescriptorSetAllocateInfo setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = this->descriptorPool; // Pool to allocate descriptorset from 
	setAllocInfo.descriptorSetCount = static_cast<uint32_t>(this->swapchainImages.size()); // Number fo sets o allocate
	setAllocInfo.pSetLayouts = setLayouts.data(); // Layouts to allocate sets (1:1 relation)

	// Allocate descriptor sets (multipl)
	VkResult result = vkAllocateDescriptorSets(this->mainDevice.logicalDevice, &setAllocInfo, this->descriptorSets.data());
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets");
	}

	// Update all of descriptor set bindings
	for (size_t i = 0; i < this->swapchainImages.size(); i++) {
		// - View Projection Descriptor
		// BUffer info and data offset info
		VkDescriptorBufferInfo vpBufferInfo = {};
		vpBufferInfo.buffer = this->vpUniformBuffer[i]; // BUffer to get the data from
		vpBufferInfo.offset = 0; // Position of start of data
		vpBufferInfo.range = sizeof(UboViewProjection); // Size of data

		// Data about connection between bindign and buffer
		VkWriteDescriptorSet vpSetWrite = {};
		vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vpSetWrite.dstSet = this->descriptorSets[i]; // Descriptor set to update
		vpSetWrite.dstBinding = 0; // The binding in the vertex shader file to which its connected to
		vpSetWrite.dstArrayElement = 0; // Index in array to update
		vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Type of descriptor
		vpSetWrite.descriptorCount = 1; // Amount to update
		vpSetWrite.pBufferInfo = &vpBufferInfo; // Information of buffer data info to biond;

		//// NO LONGER USED BELOW BUT KEEPING FOR REFERENCE, AS THAT'S HOW MODEL WAS DONE VIA DYNAMIC BUFFERS
		//// - Model Descriptor
		//// Model Buffer Binding Info
		//VkDescriptorBufferInfo modelBufferInfo = {};
		//modelBufferInfo.buffer = this->modelDynamicUniformBuffer[i];
		//modelBufferInfo.offset = 0;
		//modelBufferInfo.range = this->modelUniformAlignment;

		//VkWriteDescriptorSet modelSetWrite = {};
		//modelSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		//modelSetWrite.dstSet = this->descriptorSets[i];
		//modelSetWrite.dstBinding = 1;
		//modelSetWrite.dstArrayElement = 0;
		//modelSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		//modelSetWrite.descriptorCount = 1;
		//modelSetWrite.pBufferInfo = &modelBufferInfo;

		// List of descriptor set writes
		std::vector<VkWriteDescriptorSet> setWrites = { vpSetWrite };

		// Update the descriptor sets with new buffer/binding info
		vkUpdateDescriptorSets(
			this->mainDevice.logicalDevice, 
			static_cast<uint32_t>(setWrites.size()), 
			setWrites.data(), 
			0, 
			nullptr);
	}
}

void VulkanRenderer::updateUniformBuffers(uint32_t imageIndex)
{
	// Copy VP Data
	void* data;
	vkMapMemory(this->mainDevice.logicalDevice, this->vpUniformBufferMemory[imageIndex], 0, sizeof(UboViewProjection), 0, &data);
	memcpy(data, &this->uboViewProjection, sizeof(UboViewProjection));
	vkUnmapMemory(this->mainDevice.logicalDevice, this->vpUniformBufferMemory[imageIndex]);

	//// NO LONGER USED BELOW BUT KEEPING FOR REFERENCE, AS THAT'S HOW MODEL WAS DONE VIA DYNAMIC BUFFERS
	//// Copy Model data
	//for (size_t i = 0; i < this->meshList.size(); i++) {
	//	// Here we first take the address of the modelTransfer space, point to the relevant address of the current mesh 
	//	//	by adding the number of objects multipllied by their size, and retunring a pointer to the UboModel
	//	Model* thisModel = (Model*)((uint64_t)modelTransferSpace + (i * this->modelUniformAlignment));
	//	// Then assign the respective mesh model to the pointer location found above
	//	*thisModel = meshList[i].getModel();
	//}

	//// Map the list of model data
	//data = nullptr;
	//vkMapMemory(this->mainDevice.logicalDevice, this->modelDynamicUniformBufferMemory[imageIndex], 0, this->modelUniformAlignment * this->meshList.size(), 0, &data);
	//memcpy(data, modelTransferSpace, this->modelUniformAlignment * this->meshList.size());
	//vkUnmapMemory(this->mainDevice.logicalDevice, this->modelDynamicUniformBufferMemory[imageIndex]);
}

void VulkanRenderer::recordCommands(uint32_t currentImage)
{
	// Information about how to begin each command buffer
	VkCommandBufferBeginInfo bufferBeginInfo = {};
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	// The below is no longer necessary, as we no longer have a command buffer used more than once, if you don't have fences, then it's important to have if no fences are in place a loads of command buffers are stacked on after other
	//bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Buffer can be resubmitted when it has already been submitted and is awaiting execution

	// Information about how to begin a render pass (only needed for graphical applications)
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.renderPass = this->renderPass; // Render pass to begin
	renderPassBeginInfo.renderArea.offset = { 0, 0 }; // Start point of render pass in pixels (in case we want to star in mid of screen)
	renderPassBeginInfo.renderArea.extent = swapchainExtent; // Size of region to run render pass on (starting at offset)
	// WHen it's cleared we want to specify what it's being cleared to, and we need to define both colour and depth stencil
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.6f, 0.65f, 0.4f, 1.0f };
	clearValues[1].depthStencil.depth = 1.0f;

	renderPassBeginInfo.pClearValues = clearValues.data(); // List of clear values 
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	
	renderPassBeginInfo.framebuffer = this->swapchainFramebuffers[currentImage];
			
	// Start recording commands to comamnd buffer
	VkResult result = vkBeginCommandBuffer(this->commandBuffers[currentImage], &bufferBeginInfo);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to start recording a command buffer");
	}
	// Begin render pass
		vkCmdBeginRenderPass(this->commandBuffers[currentImage], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Bind pipeline to be used in render pass
		vkCmdBindPipeline(this->commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, this->graphicsPipeline);

		// After we bind the pipeline we can bind our vertex buffers

		for (size_t j = 0; j < this->meshList.size(); j++) {
			VkBuffer vertexBuffers[] = { this->meshList[j].getVertexBuffer() }; // Buffers to bind
			VkDeviceSize offsets[] = { 0 }; // Offsets into buffers being bound (one for each of the buffers)
			// Command to bind vertex buffer before drawing with them - parameter defs:
			// Command buffer: Command buffer to bind the vertex buffers to 
			// firstBinding: The binding based on the shader which is (binding = 0 , locaiton = <x>) by default
			// bindingCount: How many bindings to iterate through (in this case we only have 1)
			// pBuffers: This are the vertex buffers that we defined in the create function
			// pOffsets: This are the offsets for each of the buffers
			vkCmdBindVertexBuffers(this->commandBuffers[currentImage], 0, 1, vertexBuffers, offsets);

			// Binding mesh index buffers (note that we can only bind one index buffer - for multiple vertex buffers)
			vkCmdBindIndexBuffer(this->commandBuffers[currentImage], this->meshList[j].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			//// NO LONGER USED BELOW BUT KEEPING FOR REFERENCE, AS THAT'S HOW MODEL WAS DONE VIA DYNAMIC BUFFERS
			//// Dynamic offset amount
			//uint32_t dynamicOffset = static_cast<uint32_t>(this->modelUniformAlignment) * j;

			// Push constants to given shader stage directly (no buffer)
			vkCmdPushConstants(
				this->commandBuffers[currentImage],
				this->pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT, // Stage to push constants to
				0, // Offset of push constants to update
				sizeof(Model), // size of the model being pushed
				&this->meshList[j].getModel() // Actual data being pushed (can be array hence &)
			);

			std::array<VkDescriptorSet, 2> descriptorSetGroup = {
				this->descriptorSets[currentImage],
				this->samplerDescriptorSets[this->meshList[j].getTexId()]
			};

			// Bind Descriptor Sets
			vkCmdBindDescriptorSets(
				this->commandBuffers[currentImage], // Specific command buffer to bind 
				VK_PIPELINE_BIND_POINT_GRAPHICS, // Can be used in the graphics pipeline
				this->pipelineLayout, // This is how the data is coming into
				0, // Because we can have multiple sets for bidning, here we provide which one
				static_cast<uint32_t>(descriptorSetGroup.size()), // How many descriptor sets for each draw
				descriptorSetGroup.data(), // Point to the descrptor set that will be used here (one to one with command buffers, and not with meshes, so it will be the same for all our meshse) 
				0, // // Addresses that it's not possible to address to all our meshes
				nullptr);

			// Execute pipeline - Explanation on parameters (in order as per func):
			// Commandbuffer: Command buffer to attach draw command to
			// Vertexcount: Number of vertices you want to draw (if using a model, then we would have a bindvertices function) - it will basically go through in this case 3 times as we pass 3
			// Instance count: Number of instances to draw
			// First vertex: Location of the first vertex for the first one
			// FIrst instance: Which instance number to start at
			// However we're no longer using the vertex directly, we use the index buffers directly now, so see below
			//vkCmdDraw(this->commandBuffers[i], static_cast<uint32_t>(firstMesh.getVertexCount()), 1, 0, 0);

			vkCmdDrawIndexed(this->commandBuffers[currentImage], this->meshList[j].getIndexCount(), 1, 0, 0, 0);
		}

		vkCmdEndRenderPass(this->commandBuffers[currentImage]);
	// End render pass

	result = vkEndCommandBuffer(this->commandBuffers[currentImage]);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to stop recording a command buffer");
	}
}

void VulkanRenderer::getPhysicalDevice()
{
	// Enumerate physical devices the vkInstance can access
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(this->instance, &deviceCount, nullptr);

	// If no devices available, then none support vulkan!
	if (deviceCount == 0)
	{
		throw std::runtime_error("Can't find GPUs that support vulkan instance");
	}

	// Get list of Physical Devices
	std::vector<VkPhysicalDevice> deviceList(deviceCount);
	vkEnumeratePhysicalDevices(this->instance, &deviceCount, deviceList.data());

	for (const VkPhysicalDevice& device : deviceList) {
		if (this->checkDeviceSuitable(device)) {
			mainDevice.physicalDevice = device;
			break;
		}
	}

	// Get properties of our new device
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(this->mainDevice.physicalDevice, &deviceProperties);

	//// NO LONGER USED BELOW BUT KEEPING FOR REFERENCE, AS THAT'S HOW MODEL WAS DONE VIA DYNAMIC BUFFERS
	//// Get the size of the blocks for buffers
	//this->minUniformBufferOffset = deviceProperties.limits.minUniformBufferOffsetAlignment;
}

// NO LONGER USED BELOW BUT KEEPING FOR REFERENCE, AS THAT'S HOW MODEL WAS DONE VIA DYNAMIC BUFFERS
//void VulkanRenderer::allocateDynamicBufferTransferSpace()
//{
//	// Calculate alignment of model data
//	this->modelUniformAlignment = 
//		(sizeof(Model) + this->minUniformBufferOffset - 1) & ~(minUniformBufferOffset - 1);
//
//	// Create space in memory to hold dynamic buffer that is aligned to required alignment and holds MAX_OBJECts (memory allocation is expecnsive so we want to allocate the max number of objects expected)
//	this->modelTransferSpace = (Model*)_aligned_malloc(
//		this->modelUniformAlignment * MAX_OBJECTS, modelUniformAlignment);
//}

QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice physicalDevice)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyList.data());

	// Go through each queue family and check if it has at least 1 of the required types of queue
	int i = 0; // Queues are actually in order starting from 0
	for (const auto& queueFamily : queueFamilyList) {
		// First check if queue family has at least 1 queue in family (could have no queues)
		// Queue can be multiple types defined through bitfield. Need to bitwise AND with BK_QUEUE_*_BIT to check if required
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;// If queue family is valid, then get the index
		}

		// Check if queue family supports presentation
		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, this->surface, &presentationSupport);
		// A queue can be both graphics and presentation hence why its not just else if
		if (queueFamily.queueCount > 0 && presentationSupport) {
			indices.presentationFamily = i;
		}

		if (indices.isValid()) {
			break;
		}
		i++;
	}

	return indices;
}

SwapchainDetails VulkanRenderer::getSwapchainDetails(VkPhysicalDevice device)
{
	SwapchainDetails swapchainDetails;

	// -- CAPABILITIES --
	// Get the surface capabilities for the given surface on the given physical device
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, this->surface, &swapchainDetails.surfaceCapabilities);

	// -- FORMATS --
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->surface, &formatCount, nullptr);

	if (formatCount != 0) {
		swapchainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapchainDetails.formats.data());
	}

	// -- PRESENTATION MODES
	uint32_t presentationCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);

	// If presentation modes returned, get list of presentation modes
	if (presentationCount != 0) {
		swapchainDetails.presentationModes.resize(presentationCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, swapchainDetails.presentationModes.data());
	}

	return swapchainDetails;
}

bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions)
{
	// First we need to get the number of extensions to create array of correct to hold extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	// Create a list of VkExtensionProperties using count
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	// Check if given extensions are in the list of available extensiosn
	for (const char* checkExtension : *checkExtensions) {

		bool hasExtension = false;
		for (const VkExtensionProperties& extension : extensions) {
			if (strcmp(checkExtension, extension.extensionName) == 0) {
				hasExtension = true;
				break;
			}
		}
		if (!hasExtension)
		{
			std::cout << "Doesn't have extension: " << checkExtension << std::endl;
			return false;
		}
	}

	return true;
}

bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
{
	// Get device extension count
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	if (extensionCount == 0) {
		return false;
	}

	// Populate lst extension
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());

	// Check that the extension exists in device
	for (const char * const deviceExtension : deviceExtensions) {
		bool hasExtension = false;
		for (const VkExtensionProperties& extension : extensions) {
			if (strcmp(deviceExtension, extension.extensionName) == 0) {
				hasExtension = true;
			}
		}

		if (!hasExtension) {
			return false;
		}
	}
	return true;
}

bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice physicalDevice)
{
	// Information about the device itself (ID, name, type, vendor, etc)
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

	// Information about what the device can do (geo shader, tess shader, wide lines, etc)
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

	std::cout << "Checking device suitable. Vendor ID: " << deviceProperties.vendorID << std::endl;

	QueueFamilyIndices indices = this->getQueueFamilies(physicalDevice);

	bool extensionSupported = checkDeviceExtensionSupport(physicalDevice);

	bool swapchainValid = false;

	if (extensionSupported) {
		SwapchainDetails swapchainDetails = this->getSwapchainDetails(physicalDevice);
		swapchainValid = !swapchainDetails.presentationModes.empty() && !swapchainDetails.formats.empty();
	}

	return indices.isValid() && extensionSupported && swapchainValid && deviceFeatures.samplerAnisotropy;
}


// Best format is subjective but for us this is the definition:
// * Format : VK_FORMAT_R8G8B8A8_UNORM
// * colorSpace : VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	// List returns undefined if all are supported, so for simplicity we can just return the values we want
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const VkSurfaceFormatKHR& format : formats) {
		if ((format.format == VK_FORMAT_R8G8B8A8_UNORM) || (format.format == VK_FORMAT_B8G8R8A8_UNORM)) {
			return format;
		}
	}

	// If we couldn't find the most optimal ones for us, we will just return the first one in list
	return formats[0];
}

VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes)
{
	for (const VkPresentModeKHR& presentationMode : presentationModes) {
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentationMode;
		}
	}

	// As part of vulkan spec this should always have to be available
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
	// If current extent is at numeric limits, then extent can vary. Otherwise it is the size of window
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return surfaceCapabilities.currentExtent;
	}
	else {
		// If value can vary, we need to set manually to the size of the window
		int width, height;

		// We want to get the size of the window for the buffer size
		glfwGetFramebufferSize(this->window, &width, &height);

		// Create new extent using window size
		VkExtent2D newExtent = {};
		newExtent.width = static_cast<uint32_t>(width);
		newExtent.height = static_cast<uint32_t>(height);

		// Surface also defines max and min, so makes sure within boundaries by clamping val
		newExtent.width = std::max(
			surfaceCapabilities.maxImageExtent.width,
			std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
		newExtent.height = std::max(
			surfaceCapabilities.maxImageExtent.height,
			std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

		return newExtent;
	}
}

VkFormat VulkanRenderer::chooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
{
	// Loop through the options and find a compatible one
	for (VkFormat format : formats) {
		// Get properties for given format on this device
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(this->mainDevice.physicalDevice, format, &properties);

		// Depending on tiling choice, need to check for different bit flag
		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags) {
			return format;
		}
	}

	throw std::runtime_error("Failed to find matching format");
}

VkImage VulkanRenderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory)
{
	// 1. CREATE IMAGE
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D; // Type of image (1D, 2D, or 3D)
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1; // Depth of image (just 1, as there is no 3D aspect)
	imageCreateInfo.mipLevels = 1; // Level of detail, number of mipmap levels
	imageCreateInfo.arrayLayers = 1; // Can be used for cubemaps if there are multiple levesl of arrays
	imageCreateInfo.format = format; // Format type of image
	imageCreateInfo.tiling = tiling; // How image data shoudl be "tiled" (arranged in memory for optimal reading)
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Relates back to render pass, colour attachements has initial and final layout (plus what it transitions in renderpass), here we're saying the initial layout but then it will change on renderpass
	imageCreateInfo.usage = useFlags; //  Bit flags definiting what image will be used for
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT; // Number of samples for multi sampling
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Whether image can be shared between queues

	VkImage image;
	VkResult result = vkCreateImage(this->mainDevice.logicalDevice, &imageCreateInfo, nullptr, &image);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create image");
	}

	// 2. CREATE MEMORY FOR IMAGE
	// Get memory requirements for a type of image
	VkMemoryRequirements memoryRequirements = {};
	vkGetImageMemoryRequirements(this->mainDevice.logicalDevice, image, &memoryRequirements); // return / set memory requirements in param

	// Allocate memory using image requirements and user defined properties
	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(this->mainDevice.physicalDevice, memoryRequirements.memoryTypeBits, propFlags);

	result = vkAllocateMemory(this->mainDevice.logicalDevice, &memoryAllocateInfo, nullptr, imageMemory);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate memory for image");
	}

	// Connect memory to image
	result = vkBindImageMemory(this->mainDevice.logicalDevice, image, *imageMemory, 0);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to bind image to memory");
	}
	return image;
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = image; // Image to create view for
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // Type of image (1D, 2D, Cube, etc)
	viewCreateInfo.format = format; // Format of image data
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // Allows remappingof rgba components to other rgba values
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	// Subresources allow the view to view only a part of an image
	viewCreateInfo.subresourceRange.aspectMask = aspectFlags; // Which aspect of image to view (eg COLOR_BIT for viewing color)
	viewCreateInfo.subresourceRange.baseMipLevel = 0; // Start mipmap level to view from
	viewCreateInfo.subresourceRange.levelCount = 1; // Number of mipmap levels to view
	viewCreateInfo.subresourceRange.baseArrayLayer = 0; // Start array level to view from
	viewCreateInfo.subresourceRange.layerCount = 1; // Number of array levels to view

	// Create image view and return it
	VkImageView imageView;
	VkResult result = vkCreateImageView(this->mainDevice.logicalDevice, &viewCreateInfo, nullptr, &imageView);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create an image view");
	}
	return imageView;
}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code)
{
	// Shader module creation information
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(this->mainDevice.logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a shader module!");
	}

	return shaderModule;
}

int VulkanRenderer::createTextureImage(std::string fileName)
{
	// Load image file
	int width, height;
	VkDeviceSize imageSize;
	stbi_uc* imageData = this->loadTextureFile(fileName, &width, &height, &imageSize);

	// Create staging buffer to hold loaded data, ready to copy to device
	VkBuffer imageStagingBuffer;
	VkDeviceMemory imageStagingBufferMemory;
	createBuffer(this->mainDevice.physicalDevice, this->mainDevice.logicalDevice, imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&imageStagingBuffer, &imageStagingBufferMemory);

	// Copy image to staging buffer
	void* data;
	vkMapMemory(this->mainDevice.logicalDevice, imageStagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, imageData, static_cast<size_t>(imageSize));
	vkUnmapMemory(this->mainDevice.logicalDevice, imageStagingBufferMemory);

	// Free original image data
	stbi_image_free(imageData);

	// Create image to hold final texture
	VkImage texImage;
	VkDeviceMemory texImageMemory;
	texImage = createImage(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texImageMemory);

	// -- Copy data to image
	// Transition image to be DST for copay operation
	transitionImageLayout(this->mainDevice.logicalDevice, this->graphicsQueue, this->graphicsCommandPool,
		texImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Copy image data 
	copyImageBuffer(
		this->mainDevice.logicalDevice,
		this->graphicsQueue,
		this->graphicsCommandPool,
		imageStagingBuffer,
		texImage,
		width,
		height);

	// Transition image to be shader readable for shader usage
	transitionImageLayout(this->mainDevice.logicalDevice, this->graphicsQueue, this->graphicsCommandPool,
		texImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Add texture data to vector for reference
	this->textureImages.push_back(texImage);
	this->textureImageMemory.push_back(texImageMemory);

	// Destroy staging buffers
	vkDestroyBuffer(this->mainDevice.logicalDevice, imageStagingBuffer, nullptr);
	vkFreeMemory(this->mainDevice.logicalDevice, imageStagingBufferMemory, nullptr);

	// Return index of new texture image
	return textureImages.size() - 1;
}

int VulkanRenderer::createTexture(std::string fileName)
{
	// Create texture image and get its location in array
	int textureImageLoc = this->createTextureImage(fileName);

	// Create image view and add to list
	VkImageView imageView = createImageView(this->textureImages[textureImageLoc], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	this->textureImageViews.push_back(imageView);
	
	// Create texture descriptor
	int descriptorLoc = this->createTextureDescriptor(imageView);

	// Return location of set with texture
	return descriptorLoc;
}

int VulkanRenderer::createTextureDescriptor(VkImageView textureImage)
{
	VkDescriptorSet descriptorSet;

	// Descriptor set allocation info
	VkDescriptorSetAllocateInfo setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = this->samplerDescriptorPool;
	setAllocInfo.descriptorSetCount = 1;
	setAllocInfo.pSetLayouts = &this->samplerSetLayout;

	// Allocate Descriptor Sets
	VkResult result = vkAllocateDescriptorSets(this->mainDevice.logicalDevice, &setAllocInfo, &descriptorSet);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate texture descriptor");
	}

	// Texture image info
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // Image layout when in use
	imageInfo.imageView = textureImage; // IMage to bind to set
	imageInfo.sampler = this->textureSampler; // Sampler to use for set

	// Descriptor write info
	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	// Update new descriptor set
	vkUpdateDescriptorSets(this->mainDevice.logicalDevice, 1, &descriptorWrite, 0, nullptr);

	// Add descriptor set to list
	this->samplerDescriptorSets.push_back(descriptorSet);

	return this->samplerDescriptorSets.size() - 1;
}

stbi_uc* VulkanRenderer::loadTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize)
{
	// Number of channels the image uses
	int channels;

	// Load pixel data for image
	std::string fileLoc = "Textures/" + fileName;
	stbi_uc* image = stbi_load(fileLoc.c_str(), width, height, &channels, STBI_rgb_alpha);

	if (!image) {
		throw std::runtime_error("Failed to load a texture file (" + fileName + ")");
	}

	*imageSize = *width * *height * 4; // Total height times width times the number of channels

	return image;
}
