#include "VulkanRenderer.h"

VulkanRenderer::VulkanRenderer()
{
}

int VulkanRenderer::init(GLFWwindow* newWindow)
{
	// Summary of what is done in this function:
	//
	//    this->createInstance()
	//    * First we create the vulkan instance
	//    
	//    this->createSurface();
	//    * we create a surface which is what we will be drawing to which is presented to the screen
	//    
	//    this->getPhysicalDevice();
	//    * We get access to our chosen physical device (and make sure the device is compatible)
	//    
	//    this->createLogicalDevice();
	//    * We then create the logical device that can interface our physical device 
	//    
	//    this->createSwapchain();
	//    * Then we create a set of images in the swapchain, which we will be switching across, one to draw to and one to present to the surface
	//    
	//    this->createRenderPass();
	//    * Then we define the renderpass, which is what we will be calling essentially. Which we pass to renderpass and then start drawing
	//    
	//    this->createGraphicsPipeline();
	//    * Then we careate a graphics pipeline which we connect to that renderpass. The graphics pipeline will perform the draw operations
	//    * and the renderpass will pass those draw operations to the framebuffer 
	//    
	//    this->createFramebuffers();
	//    * We create the framebuffer which is the component that will receive the data from the renderpass
	//    
	//    this->createCommandPool();
	//    * We now have to create some commands that can actually specify what can be done
	//    * The command pool is the memory pool allocated for the commands to be added
	//    
	//    this->createCommandBuffers();
	//    * The command buffers actualyl have the commands
	//    
	//    this->recordCommands();
	//    * We are able to record the commands here (which are the steps to be carried out ad the commands themselves)
	//    * The commands say to start a specific renderpass with a specific pipeline and then to draw a triangle (or any relevant). When it does that it will draw from the graphics pipeline specified through the renderpass into the buffer
	//    
	//    this->createSynchronization();
	//    * Finally we create the locks that will be used to ensure that when the swapchain images are presented in the surface, these are done in order (by requesting locks)
	//    
	//    ...
	//    while True...
	//        draw()
	//    * Finally we are able to swap out the images on the swapchain so it's presented into the surface so it goes into the screen

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
		std::cout << "Creating graphics pipeline" << std::endl;
		this->createGraphicsPipeline();
		std::cout << "Creating framebuffers" << std::endl;
		this->createFramebuffers();
		std::cout << "Creating command pool" << std::endl;
		this->createCommandPool();

		// -- Create a mesh --
		// Vertex Data
		std::vector<Vertex> meshVertices = {
			{{-0.1, -0.4, 0.0}, {1.0f, 0.0f, 0.0f}}, // 0
			{{-0.1, 0.4, 0.0}, {0.0f, 1.0f, 0.0f}}, // 1
			{{-0.9, 0.4, 0.0}, {0.0f, 0.0f, 1.0f}}, // 2
			{{-0.9, -0.4, 0.0}, {1.0f, 1.0f, 0.0f}}, // 3
		};
		std::vector<Vertex> meshVertices2 = {
			{{0.9, -0.3, 0.0}, {1.0f, 0.0f, 0.0f}}, // 0
			{{0.9, 0.1, 0.0}, {0.0f, 1.0f, 0.0f}}, // 1
			{{0.1, 0.3, 0.0}, {0.0f, 0.0f, 1.0f}}, // 2
			{{0.1, -0.3, 0.0}, {1.0f, 1.0f, 0.0f}}, // 3
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
			&meshIndices);
		Mesh secondMesh = Mesh(
			this->mainDevice.physicalDevice,
			this->mainDevice.logicalDevice,
			this->graphicsQueue,
			this->graphicsCommandPool,
			&meshVertices2,
			&meshIndices);

		this->meshList.push_back(firstMesh);
		this->meshList.push_back(secondMesh);

		std::cout << "Creating command buffers" << std::endl;
		this->createCommandBuffers();
		std::cout << "Recording commands" << std::endl;
		this->recordCommands();
		std::cout << "Creating synchronisation" << std::endl;
		this->createSynchronization();
	}
	catch (const std::runtime_error& e) {
		std::cout << "ERROR" << e.what() << std::endl;
		printf("Error: %s\n", e.what());
		return EXIT_FAILURE;
	}

	std::cout << "Success init" << std::endl;
	return 0;
}

void VulkanRenderer::cleanup()
{
	// Wait until no actions are being run until destroying
	vkDeviceWaitIdle(this->mainDevice.logicalDevice);

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

	// Attachment reference uses an attachment indext that refers to in dex in the atachment list passed to renderpasscreateinfo
	VkAttachmentReference colourAttachmentReference = {};
	colourAttachmentReference.attachment = 0;
	colourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Summary of colourattachment and colouattachemntrefrence:
	// * We expect the initial layout would be layout-UNDEFINED
	// * Then when starting the subpass, we will convert it to the attachmentReference above (ATTACHMENT_OPTIMAL)
	// * Finally the image will be convertedd back into the finalLayout PRESENT_SRC_KHR

	// Information about a particualr subpass the render pass is using
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Pipeline type subpass is to be bound to
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colourAttachmentReference;

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

	VkRenderPassCreateInfo renderPassCreateInfo = { };
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colourAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassCreateInfo.pDependencies = subpassDependencies.data();

	VkResult result = vkCreateRenderPass(this->mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &this->renderPass);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass");
	}
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
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;
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
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE; // Winding to determine which side is front
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
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	// Create pipelinelayout
	VkResult result = vkCreatePipelineLayout(this->mainDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &this->pipelineLayout);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Pipeline layout");
	}

	// -- DEPTH STENCIL TESTING --

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
	pipelineCreateInfo.pDepthStencilState = nullptr;
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

void VulkanRenderer::createFramebuffers()
{
	// Resize framebuffer count to equal to swapchain image count
	swapchainFramebuffers.resize(swapchainImages.size());

	// Create a framebuffer for eachi swap chain image
	for (size_t i = 0; i < swapchainFramebuffers.size(); i++) {
		std::array<VkImageView, 1> attachments = {
			swapchainImages[i].imageView
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

void VulkanRenderer::recordCommands()
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
	VkClearValue clearValues[] = {
		{0.6f, 0.65f, 0.4f, 1.0f}
	};
	renderPassBeginInfo.pClearValues = clearValues; // List of clear values 
	renderPassBeginInfo.clearValueCount = 1;
	
	for (size_t i = 0; i < this->commandBuffers.size(); i++) {
		renderPassBeginInfo.framebuffer = this->swapchainFramebuffers[i];
			
		// Start recording commands to comamnd buffer
		VkResult result = vkBeginCommandBuffer(this->commandBuffers[i], &bufferBeginInfo);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to start recording a command buffer");
		}
		// Begin render pass
			vkCmdBeginRenderPass(this->commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Bind pipeline to be used in render pass
			vkCmdBindPipeline(this->commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, this->graphicsPipeline);

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
				vkCmdBindVertexBuffers(this->commandBuffers[i], 0, 1, vertexBuffers, offsets);

				// Binding mesh index buffers (note that we can only bind one index buffer - for multiple vertex buffers)
				vkCmdBindIndexBuffer(this->commandBuffers[i], this->meshList[j].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

				// Execute pipeline - Explanation on parameters (in order as per func):
				// Commandbuffer: Command buffer to attach draw command to
				// Vertexcount: Number of vertices you want to draw (if using a model, then we would have a bindvertices function) - it will basically go through in this case 3 times as we pass 3
				// Instance count: Number of instances to draw
				// First vertex: Location of the first vertex for the first one
				// FIrst instance: Which instance number to start at
				// However we're no longer using the vertex directly, we use the index buffers directly now, so see below
				//vkCmdDraw(this->commandBuffers[i], static_cast<uint32_t>(firstMesh.getVertexCount()), 1, 0, 0);

				vkCmdDrawIndexed(this->commandBuffers[i], this->meshList[j].getIndexCount(), 1, 0, 0, 0);
			}

			vkCmdEndRenderPass(this->commandBuffers[i]);
		// End render pass

		result = vkEndCommandBuffer(this->commandBuffers[i]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to stop recording a command buffer");
		}
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
}

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

	return indices.isValid() && extensionSupported && swapchainValid;
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
