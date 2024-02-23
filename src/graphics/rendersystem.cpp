#include <graphics/rendersystem.h>

#include <graphics/vulkan/renderingdevice.h>
#include <graphics/vulkan/utilities.h>
#include <core/engine.h>

namespace bennu {

RenderSystem* RenderSystem::getSingleton() {
	static RenderSystem singleton;
	return &singleton;
}

void RenderSystem::initialize() {
	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();

	depthRenderer.setupDescriptorSetLayouts();
	forwardRenderer.setupDescriptorSetLayouts();

	updateRenderSurfaces();

	std::vector<VkPipelineShaderStageCreateInfo> prepassShaders = {
		rd->loadSPIRVShader("../src/graphics/shaders/depth.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, &shaderModules)
	};
	depthRenderer.createRenderPipelines(prepassShaders);

	std::vector<VkPipelineShaderStageCreateInfo> forwardShaders = {
		rd->loadSPIRVShader("../src/graphics/shaders/forward.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, &shaderModules),
		rd->loadSPIRVShader("../src/graphics/shaders/forward.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, &shaderModules)
	};
	forwardRenderer.createRenderPipelines(forwardShaders);

	depthRenderer.createCommandBuffers();
	forwardRenderer.createCommandBuffers();

	createSyncObjects();

	uniformBuffers.reserve(MAX_FRAME_LAG);
	for (int i = 0; i < MAX_FRAME_LAG; i++) {
		uniformBuffers.emplace_back(sizeof(GlobalUniforms));
	}
}

void RenderSystem::createSyncObjects() {
	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();
	VkDevice device = rd->getDevice();

	presentCompleteSemaphores.resize(MAX_FRAME_LAG);
	renderCompleteSemaphores.resize(MAX_FRAME_LAG);
	inFlightFences.resize(MAX_FRAME_LAG);
	depthPrePassCompleteSemaphores.resize(MAX_FRAME_LAG);
	depthPassFences.resize(MAX_FRAME_LAG);
	clusteringInFlightFences.resize(MAX_FRAME_LAG);
	clusteringCompleteSemaphores.resize(MAX_FRAME_LAG);

	VkSemaphoreCreateInfo semaphoreCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr
	};

	VkFenceCreateInfo fenceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	for (uint32_t i = 0; i < MAX_FRAME_LAG; i++) {
		CHECK_VKRESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &presentCompleteSemaphores[i]));

		CHECK_VKRESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderCompleteSemaphores[i]));

		CHECK_VKRESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &inFlightFences[i]));

		CHECK_VKRESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &depthPrePassCompleteSemaphores[i]));

		CHECK_VKRESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &depthPassFences[i]));

		CHECK_VKRESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &clusteringCompleteSemaphores[i]));

		CHECK_VKRESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &clusteringInFlightFences[i]));
	}
}

void RenderSystem::update(const Scene& scene) {
	updateDescriptorSets(scene);
}

void RenderSystem::updateDescriptorSets(const Scene& scene) {
	std::vector<ForwardRendererBuffersInfo> frBuffersInfo;
	for (size_t i = 0; i < MAX_FRAME_LAG; i++) {
		ForwardRendererBuffersInfo info{
			.uniformSize = sizeof(GlobalUniforms),
			.uniforms = &uniformBuffers[i],
			.clusterGenSize = sizeof(ClusterGenData),
			.clusterGen = clusterGenDataBuffer.get(),
			.lightIndicesSize = (uint32_t)(clusterBuilder.getNumClusters() * clusterBuilder.getMaxLightsPerTile() * sizeof(uint32_t)),
			.lightIndices = lightIndicesBuffer.get(),
			.lightGridSize = (uint32_t)(clusterBuilder.getNumClusters() * 2 * sizeof(uint32_t)),
			.lightGrid = lightGridBuffer.get()
		};
		frBuffersInfo.push_back(info);
	}
	forwardRenderer.createDescriptorSets(scene, frBuffersInfo);

	std::vector<DepthRendererBuffersInfo> dBuffersInfo;
	for (size_t i = 0; i < MAX_FRAME_LAG; i++) {
		DepthRendererBuffersInfo info{
			.uniformSize = sizeof(GlobalUniforms),
			.uniforms = &uniformBuffers[i],
		};
		dBuffersInfo.push_back(info);
	}
	depthRenderer.createDescriptorSets(scene, dBuffersInfo);

	// TODO: cluster builder?
}

void RenderSystem::updateRenderSurfaces() {
	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();
	rd->deviceWaitIdle();
	cleanupRenderSurfaces();

	rd->updateSwapchain(&width, &height);
	vkw::Swapchain swapChain = rd->getSwapChain();

	vkw::Texture2D* color = new vkw::Texture2D({ width, height }, nullptr, 0, swapChain.getFormat(),
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_REPEAT, rd->getMSAASamples());

	vkw::AttachmentInfo attachment{ color };
	forwardRenderer.renderTarget.addColorAttachment(attachment);

	vkw::TextureDepth* depth = new vkw::TextureDepth({ width, height }, rd->getMSAASamples());
	depthTexture = std::unique_ptr<vkw::Texture>(depth);

	attachment = vkw::AttachmentInfo{ depthTexture.get() };
	attachment.loadAction = VK_ATTACHMENT_LOAD_OP_LOAD;
	attachment.storeAction = VK_ATTACHMENT_STORE_OP_STORE;
	forwardRenderer.renderTarget.setDepthStencilAttachment(attachment);

	attachment = vkw::AttachmentInfo{ depthTexture.get() };
	depthRenderer.renderTarget.setDepthStencilAttachment(attachment);

	attachment = vkw::AttachmentInfo{ &swapChain };	// TODO: potential problem? pointer goes out of scope after function
	attachment.loadAction = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	forwardRenderer.renderTarget.addColorResolveAttachment(attachment);

	depthRenderer.setupRenderPass();
	forwardRenderer.setupRenderPass();

	uint32_t imageCount = swapChain.getImageCount();
	forwardRenderer.setupTargetFramebuffers(imageCount, width, height);
	depthRenderer.setupTargetFramebuffers(1, width, height);

	Engine::getSingleton()->getCamera()->updateViewportSize(width, height);
}

void RenderSystem::cleanupRenderSurfaces() {
	forwardRenderer.cleanupRenderSurface();
	depthRenderer.cleanupRenderSurface();
}

}  // namespace bennu