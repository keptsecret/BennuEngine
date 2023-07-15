#ifndef BENNU_SWAPCHAIN_H
#define BENNU_SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include <graphics/texture.h>

#include <glfw/glfw3.h>
#include <vector>
#include <memory>

namespace bennu {

namespace vkw {

class VulkanContext;
class RenderingDevice;

class Swapchain {
public:
	void connect(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
	void initialize(VkInstance instance, GLFWwindow* window);
	void update(GLFWwindow* window);

	VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex);
	void cleanup();

private:
	friend VulkanContext;
	friend RenderingDevice;

	uint32_t width = 0;
	uint32_t height = 0;
	VkFormat colorFormat;
	VkColorSpaceKHR colorSpace;
	uint32_t imageCount = 0;

	VkInstance instance = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
};

}  // namespace vkw

}  // namespace bennu

#endif	// BENNU_SWAPCHAIN_H
