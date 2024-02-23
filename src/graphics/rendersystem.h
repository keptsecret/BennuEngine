#ifndef BENNU_RENDERSYSTEM_H
#define BENNU_RENDERSYSTEM_H

#include <graphics/clusterbuilder.h>
#include <graphics/depthrenderer.h>
#include <graphics/forwardrenderer.h>

namespace bennu {

class RenderSystem {
protected:
	RenderSystem() {}

public:
	~RenderSystem();
	static RenderSystem* getSingleton();

	void initialize();
	void update(const Scene& scene);
	void render();

	uint32_t getMaxFrameLag() const { return MAX_FRAME_LAG; }

private:
	void createSyncObjects();

	// called when scene changes - descriptor sets
	void updateDescriptorSets(const Scene& scene);

	// called every time frame size changes
	void updateRenderSurfaces();
	void cleanupRenderSurfaces();

	uint32_t width = 1920;
	uint32_t height = 1080;
	const uint32_t MAX_FRAME_LAG = 2;
	bool windowResized = false;

	uint32_t frameIndex = 0;
	uint32_t currentBuffer = 0;

	ClusterBuilder clusterBuilder;
	DepthRenderer depthRenderer;
	ForwardRenderer forwardRenderer;

	// Resources
	std::vector<vkw::UniformBuffer> uniformBuffers;
	std::unique_ptr<vkw::Texture> depthTexture;
	std::unique_ptr<vkw::StorageBuffer> clusterGenDataBuffer, lightIndicesBuffer, lightGridBuffer;	// might use vector

	std::vector<VkShaderModule> shaderModules;

	std::vector<VkSemaphore> depthPrePassCompleteSemaphores;
	std::vector<VkFence> depthPassFences;
	std::vector<VkFence> clusteringInFlightFences;
	std::vector<VkSemaphore> clusteringCompleteSemaphores;
	std::vector<VkSemaphore> presentCompleteSemaphores;
	std::vector<VkSemaphore> renderCompleteSemaphores;
	std::vector<VkFence> inFlightFences;
};

}  // namespace bennu

#endif	// BENNU_RENDERSYSTEM_H
