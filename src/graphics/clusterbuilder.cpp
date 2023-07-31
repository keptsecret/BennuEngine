#include <graphics/clusterbuilder.h>

#include <core/engine.h>
#include <graphics/utilities.h>
#include <graphics/renderingdevice.h>

namespace bennu {

void ClusterBuilder::setupBuffers() {
	clusterBoundsGridBuffer = std::make_unique<vkw::StorageBuffer>(numClusters * sizeof(AABB));
	clusterGenDataBuffer = std::make_unique<vkw::StorageBuffer>(sizeof(ClusterGenData));

	// Indices of active lights inside a cluster
	lightIndicesBuffer = std::make_unique<vkw::StorageBuffer>(numClusters * maxLightsPerTile * sizeof(uint32_t));

	// Each tile holds 1. number of lights in the grid and 2. offset of light index list to begin reading indices from
	lightGridBuffer = std::make_unique<vkw::StorageBuffer>(numClusters * 2 * sizeof(uint32_t));

	lightIndexGlobalCountBuffer = std::make_unique<vkw::StorageBuffer>(sizeof(uint32_t));
}

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
		if (clusterGenDataBuffer) {
			clusterGenDataBuffer.reset();
		}

		clusterBoundsGridBuffer = std::make_unique<vkw::StorageBuffer>(sizeof(AABB) * clusterGrids.size());
		clusterGenDataBuffer = std::make_unique<vkw::StorageBuffer>(sizeof(ClusterGenData));
	}
	clusterBoundsGridBuffer->update(clusterGrids.data());
	clusterGenDataBuffer->update(&genData);
}

void ClusterBuilder::createDescriptorSets(const Scene& scene) {
	VkDescriptorSetLayoutBinding clusterBoundsBufferBinding{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding clusterGenBufferBinding{
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr
	};

	VkDescriptorSetLayoutBinding lightBufferBinding{
		.binding = 2,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding lightIndicesBufferBinding{
		.binding = 3,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding lightGridBufferBinding{
		.binding = 4,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding lightGlobalBufferBinding{
		.binding = 5,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr
	};

	std::array<VkDescriptorSetLayoutBinding, 6> bindings = { clusterBoundsBufferBinding, clusterGenBufferBinding,
		lightBufferBinding, lightIndicesBufferBinding, lightGridBufferBinding, lightGlobalBufferBinding };

	VkDevice device = vkw::RenderingDevice::getSingleton()->getDevice();
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = (uint32_t)bindings.size(),
		.pBindings = bindings.data()
	};
	CHECK_VKRESULT(vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &clusterLightDescriptorSetLayout));

	VkDescriptorSetAllocateInfo allocateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = vkw::RenderingDevice::getSingleton()->getDescriptorPool(),
		.descriptorSetCount = 1,
		.pSetLayouts = &clusterLightDescriptorSetLayout
	};
	CHECK_VKRESULT(vkAllocateDescriptorSets(device, &allocateInfo, &clusterLightDescriptorSet));

	std::vector<VkWriteDescriptorSet> writeDescriptorSets{};
	{
		VkDescriptorBufferInfo clusterBoundsBufferInfo{
			.buffer = clusterBoundsGridBuffer->getBuffer(),
			.offset = 0,
			.range = numClusters * sizeof(AABB)
		};
		VkWriteDescriptorSet writeDescriptorSet = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = clusterLightDescriptorSet,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &clusterBoundsBufferInfo
		};
		writeDescriptorSets.push_back(writeDescriptorSet);
	}
	{
		VkDescriptorBufferInfo clusterGenBufferInfo{
			.buffer = clusterGenDataBuffer->getBuffer(),
			.offset = 0,
			.range = sizeof(ClusterGenData)
		};
		VkWriteDescriptorSet writeDescriptorSet = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = clusterLightDescriptorSet,
			.dstBinding = 1,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &clusterGenBufferInfo
		};
		writeDescriptorSets.push_back(writeDescriptorSet);
	}

	{
		VkDescriptorBufferInfo lightBufferInfo{
			.buffer = scene.getPointLightsBuffer()->getBuffer(),
			.offset = 0,
			.range = scene.getNumLights() * sizeof(PointLight)
		};
		VkWriteDescriptorSet writeDescriptorSet = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = clusterLightDescriptorSet,
			.dstBinding = 2,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &lightBufferInfo
		};
		writeDescriptorSets.push_back(writeDescriptorSet);
	}
	{
		VkDescriptorBufferInfo lightIndicesBufferInfo{
			.buffer = lightIndicesBuffer->getBuffer(),
			.offset = 0,
			.range = numClusters * maxLightsPerTile * sizeof(uint32_t)
		};
		VkWriteDescriptorSet writeDescriptorSet = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = clusterLightDescriptorSet,
			.dstBinding = 3,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &lightIndicesBufferInfo
		};
		writeDescriptorSets.push_back(writeDescriptorSet);
	}
	{
		VkDescriptorBufferInfo lightGridBufferInfo{
			.buffer = lightGridBuffer->getBuffer(),
			.offset = 0,
			.range = numClusters * 2 * sizeof(uint32_t)
		};
		VkWriteDescriptorSet writeDescriptorSet = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = clusterLightDescriptorSet,
			.dstBinding = 4,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &lightGridBufferInfo
		};
		writeDescriptorSets.push_back(writeDescriptorSet);
	}
	{
		VkDescriptorBufferInfo lightGlobalBufferInfo{
			.buffer = lightIndicesBuffer->getBuffer(),
			.offset = 0,
			.range = sizeof(uint32_t)
		};
		VkWriteDescriptorSet writeDescriptorSet = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = clusterLightDescriptorSet,
			.dstBinding = 5,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &lightGlobalBufferInfo
		};
		writeDescriptorSets.push_back(writeDescriptorSet);
	}
	vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
}

void ClusterBuilder::createPipelines() {
	VkDevice device = vkw::RenderingDevice::getSingleton()->getDevice();

	VkShaderModule clusterLightShaderModule = vkw::utils::loadShader("../src/graphics/shaders/clusterLight.comp.spv", device);
	VkPipelineShaderStageCreateInfo clusterLightShaderStageInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.module = clusterLightShaderModule,
		.pName = "main"
	};

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &clusterLightDescriptorSetLayout
	};
	CHECK_VKRESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &clusterLightPipelineLayout));

	VkComputePipelineCreateInfo pipelineCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.stage = clusterLightShaderStageInfo,
		.layout = clusterLightPipelineLayout
	};
	CHECK_VKRESULT(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &clusterLightPipeline));
}

}  // namespace bennu