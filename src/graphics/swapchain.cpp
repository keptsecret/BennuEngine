#include <graphics/swapchain.h>

#include <stdexcept>
#include <limits>
#include <algorithm>
#include <array>

namespace bennu {

namespace vkw {

void Swapchain::connect(VkInstance inst, VkPhysicalDevice physDevice, VkDevice dev) {
	instance = inst;
	physicalDevice = physDevice;
	device = dev;
}

void Swapchain::initialize(VkInstance inst, GLFWwindow *window) {
	VkResult err = glfwCreateWindowSurface(inst, window, nullptr, &surface);
	if (err != VK_SUCCESS) {
		throw std::runtime_error("ERROR::Swapchain:initialize: failed to create window surface!");
	}
}

void Swapchain::update(GLFWwindow *window) {
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes;
	if (presentModeCount != 0) {
		presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());
	}

	VkExtent2D extent;
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		extent = surfaceCapabilities.currentExtent;
	} else {
		int w, h;
		glfwGetFramebufferSize(window, &w, &h);

		extent.width = std::clamp((uint32_t)w, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
		extent.height = std::clamp((uint32_t)h, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
	}
	width = extent.width;
	height = extent.height;

	///< choose MAILBOX mode by default for now
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const auto& presentMode : presentModes) {
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			swapchainPresentMode = presentMode;
			break;
		}
	}

	uint32_t desiredImageCount = surfaceCapabilities.minImageCount + 1;
	if (surfaceCapabilities.maxImageCount > 0 && desiredImageCount > surfaceCapabilities.maxImageCount) {
		desiredImageCount = surfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.surface = surface,
		.minImageCount = desiredImageCount,
		.imageFormat = colorFormat,
		.imageColorSpace = colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,			///< check if graphics and present queues different
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.preTransform = surfaceCapabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,	///< may need changing
		.presentMode = swapchainPresentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};

	VkResult err = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);
	if (err != VK_SUCCESS) {
		throw std::runtime_error("ERROR::Swapchain:update: failed to create swap chain!");
	}

	// Create images
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
	swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

	// Create image views
	swapchainImageViews.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; i++) {
		VkImageViewCreateInfo viewCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = swapchainImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = colorFormat,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_R,
				.g = VK_COMPONENT_SWIZZLE_G,
				.b = VK_COMPONENT_SWIZZLE_B,
				.a = VK_COMPONENT_SWIZZLE_A
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		err = vkCreateImageView(device, &viewCreateInfo, nullptr, &swapchainImageViews[i]);
		if (err != VK_SUCCESS) {
			throw std::runtime_error("ERROR::Swapchain:update: failed to create image views!");
		}
	}
}

VkResult Swapchain::acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex) {
	return vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, presentCompleteSemaphore, VK_NULL_HANDLE, imageIndex);
}

void Swapchain::cleanup() {
	for (auto& imageView : swapchainImageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(device, swapchain, nullptr);
}

}  // namespace vkw

}  // namespace bennu