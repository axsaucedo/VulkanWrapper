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
		std::cout << "Creating surface" << std::endl;
		this->createSurface();
	}
	catch (const std::runtime_error& e) {
		std::cout << "ERROR" << e.what() << std::endl;
		printf("Error: %s\n", e.what());
		return EXIT_FAILURE;
	}

	return 0;
}

void VulkanRenderer::cleanup()
{
	vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
	vkDestroyDevice(this->mainDevice.logicalDevice, nullptr);
	vkDestroyInstance(instance, nullptr);
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

	return indices.isValid() && extensionSupported;
}
