#ifndef BENNU_CLUSTERBUILDER_H
#define BENNU_CLUSTERBUILDER_H

#include <scene/scene.h>

#include <glm/glm.hpp>

namespace bennu {

struct ClusterGenData {
	glm::mat4 inverseProjectionMat;
	unsigned int tileSizes[4];
	unsigned int screenWidth;
	unsigned int screenHeight;
	float sliceScalingFactor;
	float sliceBiasFactor;
};

class ClusterBuilder {
public:
	void initialize(const Scene& scene);
	void destroy();

	void compute();

	uint32_t getNumClusters() const { return numClusters; }
	uint32_t getMaxLightsPerTile() const { return maxLightsPerTile; }

	std::vector<vkw::StorageBuffer*> getExternalBuffers() const { return { clusterGenDataBuffer.get(), lightIndicesBuffer.get(), lightGridBuffer.get() }; }
	VkSemaphore getCompleteSemaphore() const { return clusteringCompleteSemaphore; }

private:
	void setupBuffers();
	void computeClusterGrids(bool rebuildBuffers = true);
	void createDescriptorSets(const Scene& scene);
	void createPipelines();

	void createCommandBuffers();
	void createSyncObjects();
	void buildCommandBuffer();

	void updateUniforms();

	const glm::uvec3 gridDims{ 16, 9, 24 };
	const uint32_t numClusters = gridDims.x * gridDims.y * gridDims.z;
	const uint32_t maxLightsPerTile = 4;

	std::unique_ptr<vkw::UniformBuffer> uniformBuffer;
	std::unique_ptr<vkw::StorageBuffer> clusterBoundsGridBuffer, clusterGenDataBuffer;
	std::unique_ptr<vkw::StorageBuffer> lightIndicesBuffer, lightGridBuffer, lightIndexGlobalCountBuffer;

	VkDescriptorSetLayout clusterLightDescriptorSetLayout;
	VkDescriptorSet clusterLightDescriptorSet;

	VkShaderModule clusterLightShaderModule;
	VkPipelineLayout clusterLightPipelineLayout;
	VkPipeline clusterLightPipeline;

	VkCommandBuffer commandBuffer;
	VkFence clusteringInFlightFence;
	VkSemaphore clusteringCompleteSemaphore;
};

}  // namespace bennu

#endif	// BENNU_CLUSTERBUILDER_H
