#include <core/engine.h>
#include <glfw/glfw3.h>
#include <graphics/utilities.h>
#include <graphics/vulkancontext.h>

#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace bennu {

namespace vkw {

void VulkanContext::createInstance() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensionNames;
	glfwExtensionNames = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	VkApplicationInfo appInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = "Test Bennu",
		.applicationVersion = VK_MAKE_VERSION(1, 1, 0),
		.pEngineName = "Bennu Engine",
		.engineVersion = VK_MAKE_VERSION(1, 1, 0),
		.apiVersion = VK_API_VERSION_1_1
	};

	VkInstanceCreateInfo instanceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
	};

	isValidationLayersEnabled = Engine::getSingleton()->isValidationLayersEnabled();
	std::vector<const char*> enabledExtensionNames(glfwExtensionNames, glfwExtensionNames + glfwExtensionCount);
	if (isValidationLayersEnabled) {
		enabledExtensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	instanceCreateInfo.enabledExtensionCount = (uint32_t)enabledExtensionNames.size();
	instanceCreateInfo.ppEnabledExtensionNames = enabledExtensionNames.data();

	bool isDebugUtilsEnabled = std::find(enabledExtensionNames.begin(), enabledExtensionNames.end(), std::string(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) != enabledExtensionNames.end();
	VkDebugUtilsMessengerCreateInfoEXT dbgCreateInfo{};
	if (isDebugUtilsEnabled) {
		dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		dbgCreateInfo.pNext = nullptr;
		dbgCreateInfo.flags = 0;
		dbgCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		dbgCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		dbgCreateInfo.pfnUserCallback = debugMessengerCallback;
		dbgCreateInfo.pUserData = nullptr;
		instanceCreateInfo.pNext = &dbgCreateInfo;
	}

	const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
	if (isValidationLayersEnabled) {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		bool layerPresent = false;
		for (const auto& layerProperties : availableLayers) {
			if (strcmp(validationLayerName, layerProperties.layerName) == 0) {
				layerPresent = true;
				break;
			}
		}
		if (layerPresent) {
			instanceCreateInfo.enabledLayerCount = 1;
			instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
		} else {
			instanceCreateInfo.enabledLayerCount = 0;
			std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not found but was requested, validation is disabled";
		}
	}

	CHECK_VKRESULT(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));

	instanceInitialized = true;

	if (isDebugUtilsEnabled) {
		CreateUtilsDebugMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		DestroyUtilsDebugMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (CreateUtilsDebugMessengerEXT == nullptr || DestroyUtilsDebugMessengerEXT == nullptr) {
			throw std::runtime_error("ERROR::VulkanContext:createInstance: failed to initialize debug utils!");
		}

		CHECK_VKRESULT(CreateUtilsDebugMessengerEXT(instance, &dbgCreateInfo, nullptr, &debugMessenger));
	}
}

void VulkanContext::createPhysicalDevice() {
	uint32_t gpuCount = 0;
	vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr);
	if (gpuCount == 0) {
		throw std::runtime_error("ERROR::VulkanContext:initialize: no device with Vulkan support found!");
	}

	std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
	vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data());

	int32_t deviceIdx = -1;
	int selectedType = -1;
	for (uint32_t i = 0; i < gpuCount; i++) {
		VkPhysicalDeviceProperties deviceProps;
		vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProps);

		bool presentSupport = false;
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyCount, queueFamilies.data());
		for (uint32_t j = 0; j < queueFamilyCount; j++) {
			VkBool32 support = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[i], j, swapChain.surface, &support);
			if (support && (queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
				presentSupport = true;
			} else {
				continue;
			}
		}

		if (presentSupport) {
			switch (deviceProps.deviceType) {
				case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: {
					if (selectedType < 4) {
						selectedType = 4;
						deviceIdx = i;
					}
				} break;
				case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: {
					if (selectedType < 3) {
						selectedType = 3;
						deviceIdx = i;
					}
				} break;
				case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: {
					if (selectedType < 2) {
						selectedType = 2;
						deviceIdx = i;
					}
				} break;
				case VK_PHYSICAL_DEVICE_TYPE_CPU: {
					if (selectedType < 1) {
						selectedType = 1;
						deviceIdx = i;
					}
				} break;
				default: {
					if (selectedType < 0) {
						selectedType = 0;
						deviceIdx = i;
					}
				} break;
			}
		}
	}

	if (deviceIdx == -1) {
		throw std::runtime_error("ERROR::VulkanContext:createPhysicalDevice: could not find a suitable device!");
	}

	physicalDevice = physicalDevices[deviceIdx];
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	if (!checkDeviceExtensionSupport(physicalDevice)) {
		throw std::runtime_error("ERROR::VulkanContext:createPhysicalDevice: selected physical device does not support requested extensions!");
	}

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	queueFamilyProperties.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

	deviceInitialized = true;
}

void VulkanContext::createDevice() {
	std::vector<VkBool32> supportsPresent(queueFamilyProperties.size());
	for (int i = 0; i < queueFamilyProperties.size(); i++) {
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, swapChain.surface, &supportsPresent[i]);
	}

	// Try to select queue family that supports both graphics and present queues
	// Otherwise, use separate ones
	uint32_t graphicsQueueFamilyIdx = UINT32_MAX;
	uint32_t presentQueueFamilyIdx = UINT32_MAX;
	uint32_t computeQueueFamilyIdx = UINT32_MAX;
	for (int i = 0; i < queueFamilyProperties.size(); i++) {
		if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			if (graphicsQueueFamilyIdx == UINT32_MAX) {
				graphicsQueueFamilyIdx = i;
			}

			if (computeQueueFamilyIdx == UINT32_MAX && queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
				computeQueueFamilyIdx = i;
			}

			if (supportsPresent[i] == VK_TRUE) {
				graphicsQueueFamilyIdx = i;
				presentQueueFamilyIdx = i;
				if (queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
					computeQueueFamilyIdx = i;
				}
				break;
			}
		}
	}

	if (presentQueueFamilyIdx == UINT32_MAX) {
		for (int i = 0; i < queueFamilyProperties.size(); i++) {
			if (supportsPresent[i] == VK_TRUE) {
				presentQueueFamilyIdx = i;
				break;
			}
		}
	}

	if (computeQueueFamilyIdx == UINT32_MAX) {
		for (int i = 0; i < queueFamilyProperties.size(); i++) {
			if (queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
				computeQueueFamilyIdx = i;
				break;
			}
		}
	}

	graphicsQueueFamilyIndex = graphicsQueueFamilyIdx;
	presentQueueFamilyIndex = presentQueueFamilyIdx;
	separatePresentQueue = graphicsQueueFamilyIdx != presentQueueFamilyIdx;
	computeQueueFamilyIndex = computeQueueFamilyIdx;

	const float defaultQueuePriority = 0.f;
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(2);
	queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[0].pNext = nullptr;
	queueCreateInfos[0].queueFamilyIndex = graphicsQueueFamilyIndex;
	queueCreateInfos[0].queueCount = 1;
	queueCreateInfos[0].pQueuePriorities = &defaultQueuePriority;
	queueCreateInfos[0].flags = 0;

	uint32_t enabledExtensionCount = 0;
	std::vector<const char*> enabledExtensionNames(enabledDeviceExtensions.size());
	for (const auto& extension : enabledDeviceExtensions) {
		enabledExtensionNames[enabledExtensionCount++] = extension.c_str();
	}

	VkDeviceCreateInfo deviceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = queueCreateInfos.data(),
		.enabledLayerCount = 0,	 ///< ignored in new Vulkan implementations anyways
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = (uint32_t)enabledExtensionNames.size(),
		.ppEnabledExtensionNames = enabledExtensionNames.data(),
		.pEnabledFeatures = &deviceFeatures
	};
	if (separatePresentQueue) {
		queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos[1].pNext = nullptr;
		queueCreateInfos[1].queueFamilyIndex = presentQueueFamilyIndex;
		queueCreateInfos[1].queueCount = 1;
		queueCreateInfos[1].pQueuePriorities = &defaultQueuePriority;
		queueCreateInfos[1].flags = 0;
		deviceCreateInfo.queueCreateInfoCount = 2;
	}

	CHECK_VKRESULT(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));
}

void VulkanContext::initializeQueues() {
	vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
	if (!separatePresentQueue) {
		presentQueue = graphicsQueue;
	} else {
		vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);
	}
	vkGetDeviceQueue(device, computeQueueFamilyIndex, 0, &computeQueue);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, swapChain.surface, &formatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	if (formatCount != 0) {
		surfaceFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, swapChain.surface, &formatCount, surfaceFormats.data());
	}

	if (formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
		swapChain.colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
		swapChain.colorSpace = surfaceFormats[0].colorSpace;
	} else {
		// we look for VK_FORMAT_B8G8R8A8_UNORM
		// otherwise, select first available format
		swapChain.colorFormat = VK_FORMAT_UNDEFINED;
		for (const auto& surfaceFormat : surfaceFormats) {
			if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM) {
				swapChain.colorFormat = surfaceFormat.format;
				swapChain.colorSpace = surfaceFormat.colorSpace;
				break;
			}
		}

		if (swapChain.colorFormat == VK_FORMAT_UNDEFINED) {
			swapChain.colorFormat = surfaceFormats[0].format;
			swapChain.colorSpace = surfaceFormats[0].colorSpace;
		}
	}
}

void VulkanContext::updateSwapchain(GLFWwindow* window) {
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	// handle window being minimized
	while(w == 0 || h == 0) {
		glfwGetFramebufferSize(window, &w, &h);
		glfwWaitEvents();
	}

	if (swapChain.swapchain) {
		cleanupSwapchain();
	}

	swapChain.update(window);
}

void VulkanContext::cleanupSwapchain() {
	swapChain.cleanup();
}

bool VulkanContext::checkDeviceExtensionSupport(VkPhysicalDevice physDevice) {
	enabledDeviceExtensions.clear();

	///< add more required extensions here
	std::vector<std::string> requestedExtensions;
	requestedExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, availableExtensions.data());

	for (const auto& extension : availableExtensions) {
		std::string extensionName(extension.extensionName);
		if (std::find(requestedExtensions.begin(), requestedExtensions.end(), extensionName) != requestedExtensions.end()) {
			enabledDeviceExtensions.push_back(extensionName);
		}
	}

	return enabledDeviceExtensions.size() == requestedExtensions.size();
}

VkBool32 VulkanContext::debugMessengerCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {
	std::string prefix("");

	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
		prefix = "VERBOSE: ";
	} else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
		prefix = "INFO: ";
	} else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		prefix = "WARNING: ";
	} else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		prefix = "ERROR: ";
	}

	std::stringstream debugMessage;
	debugMessage << prefix << "[" << pCallbackData->messageIdNumber << "][" << pCallbackData->pMessageIdName << "] : " << pCallbackData->pMessage;

	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		std::cerr << debugMessage.str() << "\n";
	} else {
		std::cout << debugMessage.str() << "\n";
	}
	fflush(stdout);

	return VK_FALSE;
}

void VulkanContext::initialize(GLFWwindow* window) {
	createInstance();

	swapChain.initialize(instance, window);

	createPhysicalDevice();

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("ERROR::VulkanContext:initialize: could not find a suitable device!");
	}

	createDevice();

	swapChain.connect(instance, physicalDevice, device);

	initializeQueues();
}

VulkanContext::~VulkanContext() {
	cleanupSwapchain();
	vkDestroySurfaceKHR(instance, swapChain.surface, nullptr);

	if (deviceInitialized) {
		vkDestroyDevice(device, nullptr);
	}

	if (isValidationLayersEnabled) {
		DestroyUtilsDebugMessengerEXT(instance, debugMessenger, nullptr);
	}

	if (instanceInitialized) {
		vkDestroyInstance(instance, nullptr);
	}
}

}  // namespace vkw

}  // namespace bennu