#ifndef BENNU_RENDERINGDEVICE_H
#define BENNU_RENDERINGDEVICE_H

#include <glfw/glfw3.h>
#include <graphics/buffer.h>
#include <graphics/rendertarget.h>
#include <graphics/vulkancontext.h>
#include <graphics/clusterbuilder.h>
#include <scene/scene.h>

#include <array>
#include <glm/glm.hpp>

#include <scene/light.h>

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
	glm::uvec2 getWindowSize() const { return {width, height}; }

	const VkDevice& getDevice() const { return vulkanContext.device; }
	const VkPhysicalDevice& getPhysicalDevice() const { return vulkanContext.physicalDevice; }
	VkPhysicalDeviceProperties getPhysicalDeviceProperties() const { return vulkanContext.deviceProperties; }
	VkPhysicalDeviceFeatures getPhysicalDeviceFeatures() const { return vulkanContext.deviceFeatures; }

	const VkQueue& getGraphicsQueue() const { return vulkanContext.graphicsQueue; }
	const VkQueue& getComputeQueue() const { return vulkanContext.computeQueue; }

	const VkDescriptorPool& getDescriptorPool() const { return descriptorPool; }
	const VkDescriptorSetLayout& getDescriptorSetLayout(int index) const { return descriptorSetLayouts[index]; }

	const VkCommandPool& getCommandPool() const { return commandPool; }

	VkResult createBuffer(VkBuffer* buffer, VkBufferUsageFlags usageFlags, VkDeviceMemory* memory, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, const void* data = nullptr);
	VkResult createCommandBuffer(VkCommandBuffer* buffer, VkCommandBufferLevel level, bool begin);
	void commandBufferSubmitIdle(VkCommandBuffer* buffer, VkQueueFlagBits queueType);
	uint32_t getMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

private:
	void setupDescriptorSetLayouts();
	void createRenderPipelines();
	void createCommandPool();
	void createCommandBuffers();
	void createSyncObjects();

	void renderDepth();
	void buildPrepassCommandBuffer();
	void renderLighting();
	void buildRenderCommandBuffer();
	void updateGlobalBuffers();

	// TODO: place into same class?
	void updateRenderArea();
	void setupRenderPasses();
	void cleanupRenderArea();

	VkPipelineShaderStageCreateInfo loadSPIRVShader(const std::string& filename, VkShaderStageFlagBits stage);

	void createDescriptorPool();
	void createDescriptorSets();

	static void windowResizeCallback(GLFWwindow* window, int width, int height);

private:
	uint32_t width = 1920;
	uint32_t height = 1080;
	const uint32_t MAX_FRAME_LAG = 2;
	bool windowResized = false;

	GLFWwindow* window;
	VulkanContext vulkanContext;
	RenderTarget renderTarget;
	RenderTarget depthPrePassTarget;

	// Main render pass
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;	// scene buffers + material images
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;
	VkPipeline renderPipeline;

	// Depth pre-pass
	VkDescriptorSetLayout depthPassDescriptorSetLayout;
	VkPipelineLayout depthPipelineLayout;
	VkRenderPass depthPrePass;
	VkPipeline depthPipeline;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkCommandBuffer> depthPrePassCommandBuffers;
	uint32_t frameIndex = 0;

	std::vector<VkShaderModule> shaderModules;

	uint32_t currentBuffer = 0;
	std::vector<VkSemaphore> depthPrePassCompleteSemaphores;
	std::vector<VkFence> depthPassFences;
	std::vector<VkSemaphore> presentCompleteSemaphores;
	std::vector<VkSemaphore> renderCompleteSemaphores;
	std::vector<VkFence> inFlightFences;

	std::vector<UniformBuffer> uniformBuffers;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<VkDescriptorSet> depthPassDescriptorSets;

	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_8_BIT;

	// TODO: test scene
	Scene scene;
	std::unique_ptr<Texture> depthTexture;

	// TODO: test cluster builder
	ClusterBuilder clusterBuilder;
};

}  // namespace vkw

}  // namespace bennu

#endif	// BENNU_RENDERINGDEVICE_H
