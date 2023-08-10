#include <graphics/clusterbuilder.h>

#include <core/engine.h>
#include <graphics/renderingdevice.h>
#include <graphics/utilities.h>

namespace bennu {

void ClusterBuilder::initialize(const Scene& scene) {
	createCommandBuffers();
	setupBuffers();
	computeClusterGrids(false);

	createDescriptorSets(scene);
	createPipelines();

	createSyncObjects();
}

void ClusterBuilder::destroy() {
	VkDevice device = vkw::RenderingDevice::getSingleton()->getDevice();

	vkDestroyDescriptorSetLayout(device, clusterLightDescriptorSetLayout, nullptr);

	vkDestroySemaphore(device, clusteringCompleteSemaphore, nullptr);
	vkDestroyFence(device, clusteringInFlightFence, nullptr);

	vkDestroyShaderModule(device, clusterLightShaderModule, nullptr);
	vkDestroyPipeline(device, clusterLightPipeline, nullptr);
	vkDestroyPipelineLayout(device, clusterLightPipelineLayout, nullptr);
}

void ClusterBuilder::setupBuffers() {
	uniformBuffer = std::make_unique<vkw::UniformBuffer>(sizeof(glm::mat4));

	clusterBoundsGridBuffer = std::make_unique<vkw::StorageBuffer>(numClusters * sizeof(GPUBB));
	clusterGenDataBuffer = std::make_unique<vkw::StorageBuffer>(sizeof(ClusterGenData));

	// Indices of active lights inside a cluster
	lightIndicesBuffer = std::make_unique<vkw::StorageBuffer>(numClusters * maxLightsPerTile * sizeof(uint32_t));

	// Each grid holds 1. number of lights in the grid and 2. offset of light index list to begin reading indices from
	lightGridBuffer = std::make_unique<vkw::StorageBuffer>(numClusters * 2 * sizeof(uint32_t));

	lightIndexGlobalCountBuffer = std::make_unique<vkw::StorageBuffer>(sizeof(uint32_t));
}

void ClusterBuilder::updateUniforms() {
	glm::mat4 view = Engine::getSingleton()->getCamera()->getViewTransform();
	uniformBuffer->update(&view);
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
	glm::mat4 proj = camera->getProjectionTransform();
	ClusterGenData genData{
		.inverseProjectionMat = glm::inverse(proj),
		.tileSizes = {
				gridDims.x, gridDims.y, gridDims.z, tileWidth },
		.screenWidth = screenDim.x,
		.screenHeight = screenDim.y,
		.sliceScalingFactor = (float)gridDims.z / log2fn,
		.sliceBiasFactor = -((float)gridDims.z * std::log2f(camera->near_plane) / log2fn)
	};

	std::vector<GPUBB> clusterGrids(numClusters);
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
				clusterGrids[idx] = GPUBB(pmin, pmax);
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

		clusterBoundsGridBuffer = std::make_unique<vkw::StorageBuffer>(sizeof(GPUBB) * clusterGrids.size());
		clusterGenDataBuffer = std::make_unique<vkw::StorageBuffer>(sizeof(ClusterGenData));
	}
	clusterBoundsGridBuffer->update(clusterGrids.data());
	clusterGenDataBuffer->update(&genData);
}

void ClusterBuilder::createDescriptorSets(const Scene& scene) {
	VkDescriptorSetLayoutBinding globalsBufferBinding{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr
	};

	VkDescriptorSetLayoutBinding clusterBoundsBufferBinding{
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding clusterGenBufferBinding{
		.binding = 2,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr
	};

	VkDescriptorSetLayoutBinding lightBufferBinding{
		.binding = 3,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding lightIndicesBufferBinding{
		.binding = 4,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding lightGridBufferBinding{
		.binding = 5,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding lightGlobalBufferBinding{
		.binding = 6,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr
	};

	std::array<VkDescriptorSetLayoutBinding, 7> bindings = { globalsBufferBinding, clusterBoundsBufferBinding, clusterGenBufferBinding,
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
	VkDescriptorBufferInfo globalsBufferInfo{
		.buffer = uniformBuffer->getBuffer(),
		.offset = 0,
		.range = sizeof(glm::mat4)
	};
	VkWriteDescriptorSet globalsWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = clusterLightDescriptorSet,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo = &globalsBufferInfo
	};
	writeDescriptorSets.push_back(globalsWriteDescriptorSet);

	VkDescriptorBufferInfo clusterBoundsBufferInfo{
		.buffer = clusterBoundsGridBuffer->getBuffer(),
		.offset = 0,
		.range = numClusters * sizeof(AABB)
	};
	VkWriteDescriptorSet clusterBoundsWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = clusterLightDescriptorSet,
		.dstBinding = 1,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pBufferInfo = &clusterBoundsBufferInfo
	};
	writeDescriptorSets.push_back(clusterBoundsWriteDescriptorSet);
	VkDescriptorBufferInfo clusterGenBufferInfo{
		.buffer = clusterGenDataBuffer->getBuffer(),
		.offset = 0,
		.range = sizeof(ClusterGenData)
	};
	VkWriteDescriptorSet clusterGenWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = clusterLightDescriptorSet,
		.dstBinding = 2,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pBufferInfo = &clusterGenBufferInfo
	};
	writeDescriptorSets.push_back(clusterGenWriteDescriptorSet);

	VkDescriptorBufferInfo lightBufferInfo{
		.buffer = scene.getPointLightsBuffer()->getBuffer(),
		.offset = 0,
		.range = scene.getNumLights() * sizeof(PointLight)
	};
	VkWriteDescriptorSet lightsWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = clusterLightDescriptorSet,
		.dstBinding = 3,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pBufferInfo = &lightBufferInfo
	};
	writeDescriptorSets.push_back(lightsWriteDescriptorSet);
	VkDescriptorBufferInfo lightIndicesBufferInfo{
		.buffer = lightIndicesBuffer->getBuffer(),
		.offset = 0,
		.range = numClusters * maxLightsPerTile * sizeof(uint32_t)
	};
	VkWriteDescriptorSet lightIndicesWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = clusterLightDescriptorSet,
		.dstBinding = 4,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pBufferInfo = &lightIndicesBufferInfo
	};
	writeDescriptorSets.push_back(lightIndicesWriteDescriptorSet);
	VkDescriptorBufferInfo lightGridBufferInfo{
		.buffer = lightGridBuffer->getBuffer(),
		.offset = 0,
		.range = numClusters * 2 * sizeof(uint32_t)
	};
	VkWriteDescriptorSet lightGridWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = clusterLightDescriptorSet,
		.dstBinding = 5,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pBufferInfo = &lightGridBufferInfo
	};
	writeDescriptorSets.push_back(lightGridWriteDescriptorSet);
	VkDescriptorBufferInfo lightGlobalBufferInfo{
		.buffer = lightIndicesBuffer->getBuffer(),
		.offset = 0,
		.range = sizeof(uint32_t)
	};
	VkWriteDescriptorSet lightGlobalWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = clusterLightDescriptorSet,
		.dstBinding = 6,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pBufferInfo = &lightGlobalBufferInfo
	};
	writeDescriptorSets.push_back(lightGlobalWriteDescriptorSet);

	vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
}

void ClusterBuilder::createPipelines() {
	VkDevice device = vkw::RenderingDevice::getSingleton()->getDevice();

	clusterLightShaderModule = vkw::utils::loadShader("../src/graphics/shaders/clusterLight.comp.spv", device);
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

void ClusterBuilder::createCommandBuffers() {
	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();

	VkCommandBufferAllocateInfo allocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = rd->getCommandPool(),
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	CHECK_VKRESULT(vkAllocateCommandBuffers(rd->getDevice(), &allocateInfo, &commandBuffer));
}

void ClusterBuilder::createSyncObjects() {
	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();

	VkSemaphoreCreateInfo semaphoreCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr
	};
	CHECK_VKRESULT(vkCreateSemaphore(rd->getDevice(), &semaphoreCreateInfo, nullptr, &clusteringCompleteSemaphore));

	VkFenceCreateInfo fenceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};
	CHECK_VKRESULT(vkCreateFence(rd->getDevice(), &fenceCreateInfo, nullptr, &clusteringInFlightFence));
}

void ClusterBuilder::buildCommandBuffer() {
	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pInheritanceInfo = nullptr
	};

	CHECK_VKRESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	///< probably doesn't need render pass
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, clusterLightPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, clusterLightPipelineLayout, 0, 1, &clusterLightDescriptorSet, 0, 0);

	vkCmdDispatch(commandBuffer, 1, 1, 6);	// TODO: check dispatch
	CHECK_VKRESULT(vkEndCommandBuffer(commandBuffer));
}

void ClusterBuilder::computeClusterLights(const VkSemaphore& waitSemaphore) {
	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();
	VkDevice device = rd->getDevice();

	vkWaitForFences(device, 1, &clusteringInFlightFence, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &clusteringInFlightFence);

	vkResetCommandBuffer(commandBuffer, 0);
	buildCommandBuffer();
	updateUniforms();

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &waitSemaphore,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &clusteringCompleteSemaphore
	};
	CHECK_VKRESULT(vkQueueSubmit(rd->getComputeQueue(), 1, &submitInfo, clusteringInFlightFence));

	// TODO: following process waits for semaphore
}

}  // namespace bennu