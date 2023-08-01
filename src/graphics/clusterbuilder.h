#ifndef BENNU_CLUSTERBUILDER_H
#define BENNU_CLUSTERBUILDER_H

#include <graphics/renderingdevice.h>

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
	void updateUniforms();

	void compute();

private:
	void setupBuffers();
	void computeClusterGrids(bool rebuildBuffers = true);
	void createDescriptorSets(const Scene& scene);
	void createPipelines();

	void createCommandBuffers();
	void createSyncObjects();
	void buildCommandBuffer();

	const glm::uvec3 gridDims{ 16, 9, 24 };
	const uint32_t numClusters = gridDims.x * gridDims.y * gridDims.z;
	const uint32_t maxLightsPerTile = 50;

	std::unique_ptr<vkw::UniformBuffer> uniformBuffer;
	std::unique_ptr<vkw::StorageBuffer> clusterBoundsGridBuffer, clusterGenDataBuffer;
	std::unique_ptr<vkw::StorageBuffer> lightIndicesBuffer, lightGridBuffer, lightIndexGlobalCountBuffer;

	VkDescriptorSetLayout clusterLightDescriptorSetLayout;
	VkDescriptorSet clusterLightDescriptorSet;

	VkPipelineLayout clusterLightPipelineLayout;
	VkPipeline clusterLightPipeline;

	VkCommandBuffer commandBuffer;
	VkFence clusteringInFlightFence;
	VkSemaphore clusteringCompleteSemaphore;
};

}  // namespace bennu

#endif	// BENNU_CLUSTERBUILDER_H
