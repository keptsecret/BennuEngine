#ifndef BENNU_DEPTHRENDERER_H
#define BENNU_DEPTHRENDERER_H

#include <graphics/vulkan/rendertarget.h>

#include <scene/scene.h>

namespace bennu {

struct DepthRendererBuffersInfo {
	uint32_t uniformSize;
	vkw::UniformBuffer* uniforms;
};

class DepthRenderer {
public:
	void setupDescriptorSetLayouts();
	void createRenderPipelines(std::vector<VkPipelineShaderStageCreateInfo>& shaderStages);

	void setupRenderPass();
	void setupTargetFramebuffers(uint32_t count, uint32_t width, uint32_t height);
	void cleanupRenderSurface();

	void createDescriptorSets(const Scene& scene, std::vector<DepthRendererBuffersInfo> buffersInfo);

	void createCommandBuffers();
	void buildCommandBuffer(int frameIndex);

	vkw::RenderTarget renderTarget;

private:
	VkDescriptorSetLayout descriptorSetLayout;
	std::vector<VkDescriptorSet> descriptorSets;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkRenderPass renderPass;

	std::vector<VkCommandBuffer> commandBuffers;
};

}  // namespace bennu

#endif	// BENNU_DEPTHRENDERER_H
