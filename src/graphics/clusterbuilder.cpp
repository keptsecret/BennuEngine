#include <core/engine.h>
#include <graphics/clusterbuilder.h>
#include <graphics/renderingdevice.h>

namespace bennu {

glm::vec4 screenToView(glm::vec4 ss, glm::vec2 dim, glm::mat4 invProj) {
	glm::vec2 uv{ ss.x / dim.x,
		ss.y / dim.y };

	glm::vec4 clip{ glm::vec2(uv.x, uv.y) * 2.f - 1.f, ss.z, ss.w };
	glm::vec4 view = invProj * clip;
	return view / view.w;
}

glm::vec3 intersectLineZPlane(glm::vec3 A, glm::vec3 B, float zDist) {
	glm::vec3 normal{ 0, 0, 1 };

	glm::vec3 ba = B - A;
	float t = (zDist - glm::dot(normal, A)) / glm::dot(normal, ba);
	return A + t * ba;
}

void ClusterBuilder::computeClusterGrids(bool rebuildBuffers) {
	Camera* camera = Engine::getSingleton()->getCamera();
	glm::uvec2 screenDim = vkw::RenderingDevice::getSingleton()->getWindowSize();
	unsigned int tileWidth = (unsigned int)std::ceilf(screenDim.x / (float)gridDims.x);

	float log2fn = std::log2f(camera->far_plane / camera->near_plane);
	ClusterGenData genData{
		.inverseProjectionMat = glm::inverse(camera->getProjectionTransform()),
		.tileSizes = {
				gridDims.x, gridDims.y, gridDims.z, tileWidth },
		.screenWidth = screenDim.x,
		.screenHeight = screenDim.y,
		.sliceScalingFactor = (float)gridDims.z / log2fn,
		.sliceBiasFactor = -((float)gridDims.z * std::log2f(camera->near_plane) / log2fn)
	};

	std::vector<AABB> clusterGrids(numClusters);
	glm::vec3 eyePos(0.0);
	for (uint32_t z = 0; z < gridDims.z; z++) {
		for (uint32_t y = 0; y < gridDims.y; y++) {
			for (uint32_t x = 0; x < gridDims.x; x++) {
				uint32_t idx = x + y * gridDims.x + z * (gridDims.x * gridDims.y);

				glm::vec4 pmin_sS{ glm::vec2(x, y) * (float)tileWidth, -1.f, 1.f };
				glm::vec4 pmax_sS{ glm::vec2(x + 1, y + 1) * (float)tileWidth, -1.f, 1.f };

				glm::vec4 pmin_vS = screenToView(pmin_sS, { screenDim.x, screenDim.y }, genData.inverseProjectionMat);
				glm::vec4 pmax_vS = screenToView(pmax_sS, { screenDim.x, screenDim.y }, genData.inverseProjectionMat);

				float tileNear = -camera->near_plane * std::pow(camera->far_plane / camera->near_plane, z / float(gridDims.z));
				float tileFar = -camera->near_plane * std::pow(camera->far_plane / camera->near_plane, (z + 1) / float(gridDims.z));

				glm::vec3 minPointNear = intersectLineZPlane(eyePos, pmin_vS, tileNear);
				glm::vec3 minPointFar = intersectLineZPlane(eyePos, pmin_vS, tileFar);
				glm::vec3 maxPointNear = intersectLineZPlane(eyePos, pmax_vS, tileNear);
				glm::vec3 maxPointFar = intersectLineZPlane(eyePos, pmax_vS, tileFar);

				glm::vec3 pmin = glm::min(glm::min(minPointNear, minPointFar), glm::min(maxPointNear, maxPointFar));
				glm::vec3 pmax = glm::max(glm::max(minPointNear, minPointFar), glm::max(maxPointNear, maxPointFar));
				clusterGrids[idx] = AABB(pmin, pmax);
			}
		}
	}

	if (rebuildBuffers) {
		if (clusterBoundsGridBuffer) {
			clusterBoundsGridBuffer.reset();
		}

		clusterBoundsGridBuffer = std::make_unique<vkw::StorageBuffer>(sizeof(AABB) * clusterGrids.size());
	}
	clusterBoundsGridBuffer->update(clusterGrids.data());
}

}  // namespace bennu