#ifndef BENNU_RENDERINGDEVICE_H
#define BENNU_RENDERINGDEVICE_H

#include <glfw/glfw3.h>
#include <graphics/buffer.h>
#include <graphics/rendertarget.h>
#include <graphics/vulkancontext.h>
#include <scene/model.h>

#include <array>
#include <glm/glm.hpp>

namespace bennu {

namespace vkw {

class RenderingDevice {
protected:
	RenderingDevice() {}

public:
	~RenderingDevice();
	static RenderingDevice* getSingleton();

	void initialize();
	void render();

	GLFWwindow* getWindow() const { return window; }
	const VkDevice& getDevice() const { return vulkanContext.device; }
	const VkPhysicalDevice& getPhysicalDevice() const { return vulkanContext.physicalDevice; }
	VkPhysicalDeviceProperties getPhysicalDeviceProperties() const { return vulkanContext.deviceProperties; }
	VkPhysicalDeviceFeatures getPhysicalDeviceFeatures() const { return vulkanContext.deviceFeatures; }

	const VkDescriptorPool& getDescriptorPool() const { return descriptorPool; }
	const VkDescriptorSetLayout& getDescriptorSetLayout(int index) const { return descriptorSetLayouts[index]; }

	VkResult createBuffer(VkBuffer* buffer, VkBufferUsageFlags usageFlags, VkDeviceMemory* memory, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, const void* data = nullptr);
	VkResult createCommandBuffer(VkCommandBuffer* buffer, VkCommandBufferLevel level, bool begin);
	void commandBufferSubmitIdle(VkCommandBuffer* buffer, VkQueueFlagBits queueType);
	uint32_t getMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

private:
	void setupDescriptorSetLayout();
	void createRenderPipeline();
	void createCommandPool();
	void createCommandBuffers();
	void buildCommandBuffer();
	void createSyncObjects();

	void draw();
	void updateUniformBuffers();

	// TODO: place into same class?
	void updateRenderArea();
	void cleanupRenderArea();

	VkPipelineShaderStageCreateInfo loadSPIRVShader(const std::string& filename, VkShaderStageFlagBits stage);

	void createDescriptorPool();
	void createDescriptorSets();

	static void windowResizeCallback(GLFWwindow* window, int width, int height);

	// TODO: review placement
	void setupRenderPass();

private:
	uint32_t width = 800;
	uint32_t height = 600;
	const uint32_t MAX_FRAME_LAG = 2;
	bool windowResized = false;

	GLFWwindow* window;
	VulkanContext vulkanContext;
	RenderTarget renderTarget;

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	VkPipelineLayout pipelineLayout;
	std::vector<VkShaderModule> shaderModules;
	VkRenderPass renderPass;
	VkPipeline renderPipeline;
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;
	uint32_t frameIndex = 0;

	uint32_t currentBuffer = 0;
	std::vector<VkSemaphore> presentCompleteSemaphores;
	std::vector<VkSemaphore> renderCompleteSemaphores;
	std::vector<VkFence> inFlightFences;

	Model testModel;

	std::vector<UniformBuffer> uniformBuffers;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_8_BIT;
};

}  // namespace vkw

}  // namespace bennu

#endif	// BENNU_RENDERINGDEVICE_H
