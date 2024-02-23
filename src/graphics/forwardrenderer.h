#ifndef BENNU_FORWARDRENDERER_H
#define BENNU_FORWARDRENDERER_H

#include <graphics/vulkan/rendertarget.h>
#include <scene/scene.h>

namespace bennu {

struct ForwardRendererBuffersInfo {
	uint32_t uniformSize;
	vkw::UniformBuffer* uniforms;
	uint32_t clusterGenSize;
	vkw::StorageBuffer* clusterGen;
	uint32_t lightIndicesSize;
	vkw::StorageBuffer* lightIndices;
	uint32_t lightGridSize;
	vkw::StorageBuffer* lightGrid;
};

class ForwardRenderer {
public:
	void setupDescriptorSetLayouts();
	void createRenderPipelines(std::vector<VkPipelineShaderStageCreateInfo>& shaderStages);

	void setupRenderPass();
	void setupTargetFramebuffers(uint32_t count, uint32_t width, uint32_t height);
	void cleanupRenderSurface();

	void createDescriptorSets(const Scene& scene, std::vector<ForwardRendererBuffersInfo>& buffersInfo);

	void createCommandBuffers();
	void buildCommandBuffer(int frameIndex);

	vkw::RenderTarget renderTarget;

private:
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	std::vector<VkDescriptorSet> descriptorSets;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkRenderPass renderPass;

	std::vector<VkCommandBuffer> commandBuffers;
};

}  // namespace bennu

#endif	// BENNU_FORWARDRENDERER_H
