#pragma once

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Indices (locations) of queue families (if they exist at all)
struct QueueFamilyIndices {
	int graphicsFamily = -1; // Location of Graphics Queue Family
	int presentationFamily = -1; // Location of presentation queue family

	// Check if queue families are valid
	bool isValid() {
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};