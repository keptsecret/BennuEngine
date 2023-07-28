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
private:
	void computeClusterGrids(bool rebuildBuffers = true);

	const glm::uvec3 gridDims{ 16, 9, 24 };
	const uint32_t numClusters = gridDims.x * gridDims.y * gridDims.z;

	std::unique_ptr<vkw::StorageBuffer> clusterBoundsGridBuffer, clusterGenDataBuffer;
	std::unique_ptr<vkw::StorageBuffer> lightIndicesBuffer, lightGridBuffer, lightIndexGlobalCountBuffer;
};

}  // namespace bennu

#endif	// BENNU_CLUSTERBUILDER_H
