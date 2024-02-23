#include <graphics/vulkan/renderingdevice.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <graphics/vulkan/utilities.h>
#include <core/engine.h>

namespace bennu {

namespace vkw {

void RenderingDevice::initialize() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(width, height, "New window", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, windowResizeCallback);

	vulkanContext.initialize(window);

	setupDescriptorSetLayouts();
	createCommandPool();

	updateRenderArea();

	createRenderPipelines();
	createCommandBuffers();
	createSyncObjects();

	uniformBuffers.reserve(MAX_FRAME_LAG);
	for (int i = 0; i < MAX_FRAME_LAG; i++) {
		uniformBuffers.emplace_back(sizeof(GlobalUniforms));
	}

	createDescriptorPool();

	scene.loadModel("../resources/viking_room/viking_room.obj", aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_FlipUVs);
	scene.createDirectionalLight({0.1, -1, 0.1}, {1, 0, 0.1}, 1);
	scene.addPointLight({0, 0.3, 0}, {0.1, 1, 0.8}, 0.6, 3);
	//scene.addPointLight({0.4, 0.4, 0.2}, {0.3, 0.5, 0.6}, 0.3, 2);
	scene.updateSceneBufferData(true);

	clusterBuilder.initialize(scene);

	createDescriptorSets();
}

void RenderingDevice::setupDescriptorSetLayouts() {
	// Global values set layout
	VkDescriptorSetLayoutBinding globalsLayoutBinding{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.pImmutableSamplers = nullptr
	};

	VkDescriptorSetLayoutBinding directionalLayoutBinding{
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	};

	VkDescriptorSetLayoutBinding pointBufferBinding{
		.binding = 2,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	};

	VkDescriptorSetLayoutBinding clusterGenBufferBinding{
		.binding = 3,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding lightIndicesBufferBinding{
		.binding = 4,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding lightGridBufferBinding{
		.binding = 5,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	};

	std::array<VkDescriptorSetLayoutBinding, 6> globalBindings = { globalsLayoutBinding, directionalLayoutBinding, pointBufferBinding,
		clusterGenBufferBinding, lightIndicesBufferBinding, lightGridBufferBinding };

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = (uint32_t)globalBindings.size(),
		.pBindings = globalBindings.data()
	};

	VkDescriptorSetLayout globalSetLayout;
	CHECK_VKRESULT(vkCreateDescriptorSetLayout(vulkanContext.device, &layoutCreateInfo, nullptr, &globalSetLayout));
	descriptorSetLayouts.push_back(globalSetLayout);

	// Depth pre pass set layout
	{
		VkDescriptorSetLayoutCreateInfo prepassLayoutCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = 1,
			.pBindings = &globalsLayoutBinding
		};

		CHECK_VKRESULT(vkCreateDescriptorSetLayout(vulkanContext.device, &prepassLayoutCreateInfo, nullptr, &depthPassDescriptorSetLayout));
	}

	// Material set layout
	VkDescriptorSetLayoutBinding matauxLayoutBinding{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	};

	VkDescriptorSetLayoutBinding albedoLayoutBinding{
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding metallicLayoutBinding{
		.binding = 2,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding roughLayoutBinding{
		.binding = 3,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding ambientLayoutBinding{
		.binding = 4,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding normalLayoutBinding{
		.binding = 5,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	};

	std::array<VkDescriptorSetLayoutBinding, 6> matBindings = { matauxLayoutBinding, albedoLayoutBinding, metallicLayoutBinding, roughLayoutBinding, ambientLayoutBinding, normalLayoutBinding };
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = (uint32_t)matBindings.size();
	layoutCreateInfo.pBindings = matBindings.data();

	VkDescriptorSetLayout imageSetLayout;
	CHECK_VKRESULT(vkCreateDescriptorSetLayout(vulkanContext.device, &layoutCreateInfo, nullptr, &imageSetLayout));
	descriptorSetLayouts.push_back(imageSetLayout);
}

void RenderingDevice::createRenderPipelines() {
	VkPipelineShaderStageCreateInfo shaderStages[] = {
		loadSPIRVShader("../src/graphics/shaders/forward.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, &shaderModules),
		loadSPIRVShader("../src/graphics/shaders/forward.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, &shaderModules)
	};

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	auto bindingDescription = Vertex::getBindingDescription(0);
	auto attributeDescriptions = Vertex::getAttributeDescriptions(0);

	VkPipelineVertexInputStateCreateInfo vertexInputState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size(),
		.pVertexAttributeDescriptions = attributeDescriptions.data()
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	///< ensure viewport and scissors are created
	VkPipelineDynamicStateCreateInfo dynamicState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = (uint32_t)dynamicStates.size(),
		.pDynamicStates = dynamicStates.data()
	};

	VkPipelineViewportStateCreateInfo viewportState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1
	};

	VkPipelineRasterizationStateCreateInfo rasterizationState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.f
	};

	VkPipelineMultisampleStateCreateInfo multisampleState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = msaaSamples,
		.sampleShadingEnable = VK_FALSE
	};

	VkPipelineDepthStencilStateCreateInfo depthStencilState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_FALSE,
		.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.minDepthBounds = 0.f,
		.maxDepthBounds = 1.f,
//		.back = {
//				.failOp = VK_STENCIL_OP_KEEP,
//				.passOp = VK_STENCIL_OP_KEEP,
//				.compareOp = VK_COMPARE_OP_ALWAYS }
	};
//	depthStencilState.front = depthStencilState.back;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{
		.blendEnable = VK_FALSE,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	VkPipelineColorBlendStateCreateInfo colorBlendState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment
	};

	VkPushConstantRange pushConstants{
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof(MeshPushConstants)
	};

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ ///< good idea to separate this out
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = (uint32_t)descriptorSetLayouts.size(),
		.pSetLayouts = descriptorSetLayouts.data(),
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstants
	};

	CHECK_VKRESULT(vkCreatePipelineLayout(vulkanContext.device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = 2,
		.pStages = shaderStages,
		.pVertexInputState = &vertexInputState,
		.pInputAssemblyState = &inputAssemblyState,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizationState,
		.pMultisampleState = &multisampleState,
		.pDepthStencilState = &depthStencilState,
		.pColorBlendState = &colorBlendState,
		.pDynamicState = &dynamicState,
		.layout = pipelineLayout,
		.renderPass = renderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	CHECK_VKRESULT(vkCreateGraphicsPipelines(vulkanContext.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &renderPipeline));

	// Depth prepass pipeline
	{
		VkPipelineShaderStageCreateInfo prepassShaderStages[] = {
			loadSPIRVShader("../src/graphics/shaders/depth.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, &shaderModules)
		};

		VkPipelineDepthStencilStateCreateInfo prepassDepthStencilState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_LESS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.minDepthBounds = 0.f,
			.maxDepthBounds = 1.f,
//			.back = {
//					.failOp = VK_STENCIL_OP_KEEP,
//					.passOp = VK_STENCIL_OP_KEEP,
//					.compareOp = VK_COMPARE_OP_ALWAYS }
		};
//		prepassDepthStencilState.front = prepassDepthStencilState.back;

		VkPipelineLayoutCreateInfo depthPrePassPipelineLayoutCreateInfo{ ///< good idea to separate this out
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &depthPassDescriptorSetLayout,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushConstants
		};

		CHECK_VKRESULT(vkCreatePipelineLayout(vulkanContext.device, &depthPrePassPipelineLayoutCreateInfo, nullptr, &depthPipelineLayout));

		VkGraphicsPipelineCreateInfo prepassPipelineCreateInfo{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = 1,
			.pStages = prepassShaderStages,
			.pVertexInputState = &vertexInputState,
			.pInputAssemblyState = &inputAssemblyState,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizationState,
			.pMultisampleState = &multisampleState,
			.pDepthStencilState = &prepassDepthStencilState,
			.pColorBlendState = nullptr,
			.pDynamicState = &dynamicState,
			.layout = depthPipelineLayout,
			.renderPass = depthPrePass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = -1
		};

		CHECK_VKRESULT(vkCreateGraphicsPipelines(vulkanContext.device, VK_NULL_HANDLE, 1, &prepassPipelineCreateInfo, nullptr, &depthPipeline));
	}
}

void RenderingDevice::createCommandPool() {
	VkCommandPoolCreateInfo commandPoolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = vulkanContext.graphicsQueueFamilyIndex
	};

	CHECK_VKRESULT(vkCreateCommandPool(vulkanContext.device, &commandPoolCreateInfo, nullptr, &commandPool));
}

void RenderingDevice::createCommandBuffers() {
	uint32_t commandBufferCount = MAX_FRAME_LAG;
	commandBuffers.resize(commandBufferCount);

	VkCommandBufferAllocateInfo allocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = commandBufferCount
	};

	CHECK_VKRESULT(vkAllocateCommandBuffers(vulkanContext.device, &allocateInfo, commandBuffers.data()));

	depthPrePassCommandBuffers.resize(commandBufferCount);
	CHECK_VKRESULT(vkAllocateCommandBuffers(vulkanContext.device, &allocateInfo, depthPrePassCommandBuffers.data()));
}

void RenderingDevice::buildRenderCommandBuffer() {
	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pInheritanceInfo = nullptr
	};

	std::array<VkClearValue, 1> clearValues{};
	clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};

	VkRenderPassBeginInfo renderPassBeginInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass,
		.framebuffer = renderTarget.getFramebuffer(currentBuffer),
		.renderArea = {
				.offset = { 0, 0 },
				.extent = {
						.width = vulkanContext.swapChain.width,
						.height = vulkanContext.swapChain.height } },
		.clearValueCount = clearValues.size(),
		.pClearValues = clearValues.data()
	};

	CHECK_VKRESULT(vkBeginCommandBuffer(commandBuffers[frameIndex], &beginInfo));

	// Start the first sub pass specified in our default render pass setup by the base class
	// This will clear the color attachment
	vkCmdBeginRenderPass(commandBuffers[frameIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Update dynamic viewport state
	VkViewport viewport{
		.x = 0.f,
		.y = (float)vulkanContext.swapChain.height,
		.width = (float)vulkanContext.swapChain.width,
		.height = -((float)vulkanContext.swapChain.height),
		.minDepth = 0.f,
		.maxDepth = 1.f,
	};
	vkCmdSetViewport(commandBuffers[frameIndex], 0, 1, &viewport);

	// Update dynamic scissor state
	VkRect2D scissor{
		.offset = { 0, 0 },
		.extent = { vulkanContext.swapChain.width, vulkanContext.swapChain.height }
	};
	vkCmdSetScissor(commandBuffers[frameIndex], 0, 1, &scissor);

	// Bind the rendering pipeline
	// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
	vkCmdBindPipeline(commandBuffers[frameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPipeline);

	vkCmdBindDescriptorSets(commandBuffers[frameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[frameIndex], 0, nullptr);

	scene.draw(commandBuffers[frameIndex], pipelineLayout, RenderFlag::BindImages);

	// Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to
	// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system
	vkCmdEndRenderPass(commandBuffers[frameIndex]);
	CHECK_VKRESULT(vkEndCommandBuffer(commandBuffers[frameIndex]));
}

void RenderingDevice::createSyncObjects() {
	presentCompleteSemaphores.resize(MAX_FRAME_LAG);
	renderCompleteSemaphores.resize(MAX_FRAME_LAG);
	inFlightFences.resize(MAX_FRAME_LAG);
	depthPrePassCompleteSemaphores.resize(MAX_FRAME_LAG);
	depthPassFences.resize(MAX_FRAME_LAG);

	VkSemaphoreCreateInfo semaphoreCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr
	};

	VkFenceCreateInfo fenceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	for (uint32_t i = 0; i < MAX_FRAME_LAG; i++) {
		CHECK_VKRESULT(vkCreateSemaphore(vulkanContext.device, &semaphoreCreateInfo, nullptr, &presentCompleteSemaphores[i]));

		CHECK_VKRESULT(vkCreateSemaphore(vulkanContext.device, &semaphoreCreateInfo, nullptr, &renderCompleteSemaphores[i]));

		CHECK_VKRESULT(vkCreateFence(vulkanContext.device, &fenceCreateInfo, nullptr, &inFlightFences[i]));

		CHECK_VKRESULT(vkCreateSemaphore(vulkanContext.device, &semaphoreCreateInfo, nullptr, &depthPrePassCompleteSemaphores[i]));

		CHECK_VKRESULT(vkCreateFence(vulkanContext.device, &fenceCreateInfo, nullptr, &depthPassFences[i]));
	}
}

VkResult RenderingDevice::createBuffer(VkBuffer* buffer, VkBufferUsageFlags usageFlags, VkDeviceMemory* memory, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, const void* data) {
	VkBufferCreateInfo bufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usageFlags,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	VkResult err = vkCreateBuffer(vulkanContext.device, &bufferCreateInfo, nullptr, buffer);
	if (err != VK_SUCCESS) {
		return err;
	}

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(vulkanContext.device, *buffer, &memoryRequirements);
	VkMemoryAllocateInfo allocateInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = getMemoryType(memoryRequirements.memoryTypeBits, memoryPropertyFlags)
	};

	err = vkAllocateMemory(vulkanContext.device, &allocateInfo, nullptr, memory);
	if (err != VK_SUCCESS) {
		return err;
	}

	if (data != nullptr) {
		void* mapped;
		vkMapMemory(vulkanContext.device, *memory, 0, bufferCreateInfo.size, 0, &mapped);
		memcpy(mapped, data, (size_t)bufferCreateInfo.size);
		vkUnmapMemory(vulkanContext.device, *memory);
	}

	err = vkBindBufferMemory(vulkanContext.device, *buffer, *memory, 0);
	if (err != VK_SUCCESS) {
		return err;
	}

	return VK_SUCCESS;
}

uint32_t RenderingDevice::getMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	for (uint32_t i = 0; i < vulkanContext.memoryProperties.memoryTypeCount; i++) {
		if (typeFilter & (1 << i)) {
			if ((vulkanContext.memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
	}

	throw std::runtime_error("ERROR::RenderingDevice:getMemoryType: failed to find suitable memory type!");
}

VkResult RenderingDevice::createCommandBuffer(VkCommandBuffer* buffer, VkCommandBufferLevel level, bool begin) {
	VkCommandBufferAllocateInfo allocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = commandPool,
		.level = level,
		.commandBufferCount = 1
	};

	VkResult err = vkAllocateCommandBuffers(vulkanContext.device, &allocateInfo, buffer);
	if (err != VK_SUCCESS) {
		return err;
	}

	if (begin) {
		VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};

		CHECK_VKRESULT(vkBeginCommandBuffer(*buffer, &beginInfo));
	}

	return VK_SUCCESS;
}

void RenderingDevice::commandBufferSubmitIdle(VkCommandBuffer* buffer, VkQueueFlagBits queueType) {
	CHECK_VKRESULT(vkEndCommandBuffer(*buffer));

	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = buffer
	};

	VkQueue queue;
	switch (queueType) {
		case VK_QUEUE_GRAPHICS_BIT:
			queue = vulkanContext.graphicsQueue;
			break;
		default:
			queue = VK_NULL_HANDLE;
			break;
	}

	CHECK_VKRESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(vulkanContext.device, commandPool, 1, buffer);
}

VkPipelineShaderStageCreateInfo RenderingDevice::loadSPIRVShader(const std::string& filename, VkShaderStageFlagBits stage, std::vector<VkShaderModule>* shaderModules) {
	VkPipelineShaderStageCreateInfo shaderStageInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = stage,
		.module = utils::loadShader(filename, vulkanContext.device),
		.pName = "main"
	};

	shaderModules->push_back(shaderStageInfo.module);

	return shaderStageInfo;
}

void RenderingDevice::windowResizeCallback(GLFWwindow* window, int width, int height) {
	auto rd = (RenderingDevice*)glfwGetWindowUserPointer(window);
	rd->windowResized = true;
}

void RenderingDevice::updateGlobalBuffers() {
	Engine* e = Engine::getSingleton();

	GlobalUniforms ubo{
		.view = e->getCamera()->getViewTransform(),
		.projection = e->getCamera()->getProjectionTransform(),
		.cameraPosition = glm::vec4(e->getCamera()->position, 1),
		.camNear = e->getCamera()->near_plane,
		.camFar = e->getCamera()->far_plane
	};
	uniformBuffers[frameIndex].update(&ubo);

	///< scene.updateSceneBufferData();
}

void RenderingDevice::updateRenderArea() {
	vkDeviceWaitIdle(vulkanContext.device);

	cleanupRenderArea();

	vulkanContext.updateSwapchain(window);

	width = vulkanContext.swapChain.width;
	height = vulkanContext.swapChain.height;

	Texture2D* color = new Texture2D({ width, height }, nullptr, 0, vulkanContext.swapChain.colorFormat,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_REPEAT, msaaSamples);

	AttachmentInfo attachment{ color };
	renderTarget.addColorAttachment(attachment);

	TextureDepth* depth = new TextureDepth({ width, height }, msaaSamples);
	depthTexture = std::unique_ptr<Texture>(depth);

	attachment = AttachmentInfo{ depthTexture.get() };
	attachment.loadAction = VK_ATTACHMENT_LOAD_OP_LOAD;
	attachment.storeAction = VK_ATTACHMENT_STORE_OP_STORE;
	renderTarget.setDepthStencilAttachment(attachment);

	attachment = AttachmentInfo{ depthTexture.get() };
	depthPrePassTarget.setDepthStencilAttachment(attachment);

	attachment = AttachmentInfo{ &vulkanContext.swapChain };
	attachment.loadAction = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	renderTarget.addColorResolveAttachment(attachment);

	setupRenderPasses();

	uint32_t imageCount = vulkanContext.swapChain.imageCount;
	renderTarget.setupFramebuffers(imageCount, {width, height}, renderPass);
	depthPrePassTarget.setupFramebuffers(1, {width, height}, depthPrePass);

	Engine::getSingleton()->getCamera()->updateViewportSize(width, height);
}

void RenderingDevice::setupRenderPasses() {
	{	// Depth pre pass
		VkSubpassDescription subpass{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 0,
			.pDepthStencilAttachment = depthPrePassTarget.getDepthStencilReference()
		};

		VkSubpassDependency dependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
		};

		VkRenderPassCreateInfo renderPassCreateInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = depthPrePassTarget.getNumAttachmentDescriptions(),
			.pAttachments = depthPrePassTarget.getAttachmentDescriptions(),
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &dependency
		};

		CHECK_VKRESULT(vkCreateRenderPass(vulkanContext.device, &renderPassCreateInfo, nullptr, &depthPrePass));
	}
	{	// Main render pass
		VkSubpassDescription subpass{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = renderTarget.getNumColorAttachments(),
			.pColorAttachments = renderTarget.getColorAttachmentReferences(),
			.pResolveAttachments = renderTarget.getResolveAttachmentReferences(),
			.pDepthStencilAttachment = renderTarget.getDepthStencilReference()
		};

		VkSubpassDependency dependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
		};

		VkRenderPassCreateInfo renderPassCreateInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = renderTarget.getNumAttachmentDescriptions(),
			.pAttachments = renderTarget.getAttachmentDescriptions(),
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &dependency
		};

		CHECK_VKRESULT(vkCreateRenderPass(vulkanContext.device, &renderPassCreateInfo, nullptr, &renderPass));
	}
}

void RenderingDevice::cleanupRenderArea() {
	renderTarget.destroy();
	depthPrePassTarget.destroy();

	vkDestroyRenderPass(vulkanContext.device, renderPass, nullptr);
	vkDestroyRenderPass(vulkanContext.device, depthPrePass, nullptr);
}

void RenderingDevice::render() {
	updateGlobalBuffers();

	VkResult err = vulkanContext.swapChain.acquireNextImage(presentCompleteSemaphores[frameIndex], &currentBuffer);
	if (err == VK_ERROR_OUT_OF_DATE_KHR) {
		updateRenderArea();
		return;
	} else if (err != VK_SUCCESS && err != VK_SUBOPTIMAL_KHR) {
		CHECK_VKRESULT(err);
	}

	renderDepth();
	clusterBuilder.computeClusterLights(depthPrePassCompleteSemaphores[frameIndex]);
	renderLighting();

	VkPresentInfoKHR presentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &renderCompleteSemaphores[frameIndex],
		.swapchainCount = 1,
		.pSwapchains = &vulkanContext.swapChain.swapchain,
		.pImageIndices = &currentBuffer,
		.pResults = nullptr
	};

	err = vkQueuePresentKHR(vulkanContext.presentQueue, &presentInfo);
	if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR || windowResized) {
		updateRenderArea();
		windowResized = false;
	} else if (err != VK_SUCCESS) {
		CHECK_VKRESULT(err);
	}

	frameIndex += 1;
	frameIndex %= MAX_FRAME_LAG;
}

void RenderingDevice::renderDepth() {
	vkWaitForFences(vulkanContext.device, 1, &depthPassFences[frameIndex], VK_TRUE, UINT64_MAX);
	vkResetFences(vulkanContext.device, 1, &depthPassFences[frameIndex]);

	vkResetCommandBuffer(depthPrePassCommandBuffers[frameIndex], 0);
	buildPrepassCommandBuffer();

	VkSemaphore waitSemaphores[] = { presentCompleteSemaphores[frameIndex] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = waitSemaphores,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &depthPrePassCommandBuffers[frameIndex],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &depthPrePassCompleteSemaphores[frameIndex]
	};

	CHECK_VKRESULT(vkQueueSubmit(vulkanContext.graphicsQueue, 1, &submitInfo, depthPassFences[frameIndex]));
}

void RenderingDevice::renderLighting() {
	vkWaitForFences(vulkanContext.device, 1, &inFlightFences[frameIndex], VK_TRUE, UINT64_MAX);
	vkResetFences(vulkanContext.device, 1, &inFlightFences[frameIndex]);

	vkResetCommandBuffer(commandBuffers[frameIndex], 0);
	buildRenderCommandBuffer();

	VkSemaphore waitSemaphores[] = { clusterBuilder.getCompleteSemaphore() };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };	///< needs cluster data on fragment step
	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = waitSemaphores,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffers[frameIndex],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &renderCompleteSemaphores[frameIndex]
	};

	CHECK_VKRESULT(vkQueueSubmit(vulkanContext.graphicsQueue, 1, &submitInfo, inFlightFences[frameIndex]));
}

RenderingDevice::~RenderingDevice() {
	vkDeviceWaitIdle(vulkanContext.device);

	scene.unload();

	cleanupRenderArea();

	vkDestroyDescriptorPool(vulkanContext.device, descriptorPool, nullptr);
	for (int i = 0; i < descriptorSetLayouts.size(); i++) {
		vkDestroyDescriptorSetLayout(vulkanContext.device, descriptorSetLayouts[i], nullptr);
	}
	vkDestroyDescriptorSetLayout(vulkanContext.device, depthPassDescriptorSetLayout, nullptr);

	for (size_t i = 0; i < MAX_FRAME_LAG; i++) {
		vkDestroySemaphore(vulkanContext.device, presentCompleteSemaphores[i], nullptr);
		vkDestroySemaphore(vulkanContext.device, renderCompleteSemaphores[i], nullptr);
		vkDestroySemaphore(vulkanContext.device, depthPrePassCompleteSemaphores[i], nullptr);
		vkDestroyFence(vulkanContext.device, inFlightFences[i], nullptr);
		vkDestroyFence(vulkanContext.device, depthPassFences[i], nullptr);
	}

	for (auto& shaderModule : shaderModules) {
		vkDestroyShaderModule(vulkanContext.device, shaderModule, nullptr);
	}

	vkDestroyCommandPool(vulkanContext.device, commandPool, nullptr);
	vkDestroyPipeline(vulkanContext.device, renderPipeline, nullptr);
	vkDestroyPipelineLayout(vulkanContext.device, pipelineLayout, nullptr);

	vkDestroyPipeline(vulkanContext.device, depthPipeline, nullptr);
	vkDestroyPipelineLayout(vulkanContext.device, depthPipelineLayout, nullptr);

	clusterBuilder.destroy();

	glfwDestroyWindow(window);
	glfwTerminate();
}

RenderingDevice* RenderingDevice::getSingleton() {
	static RenderingDevice singleton;
	return &singleton;
}

void RenderingDevice::createDescriptorPool() {
	std::array<VkDescriptorPoolSize, 3> poolSizes{};
	poolSizes[0] = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 128
	};
	poolSizes[1] = {
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1024
	};
	poolSizes[2] = {
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 128
	};

	VkDescriptorPoolCreateInfo poolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 256,
		.poolSizeCount = poolSizes.size(),
		.pPoolSizes = poolSizes.data()
	};

	CHECK_VKRESULT(vkCreateDescriptorPool(vulkanContext.device, &poolCreateInfo, nullptr, &descriptorPool));
}

void RenderingDevice::createDescriptorSets() {
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAME_LAG, descriptorSetLayouts[0]);
	VkDescriptorSetAllocateInfo allocateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = MAX_FRAME_LAG,
		.pSetLayouts = layouts.data()
	};

	descriptorSets.resize(MAX_FRAME_LAG);
	CHECK_VKRESULT(vkAllocateDescriptorSets(vulkanContext.device, &allocateInfo, descriptorSets.data()));

	for (size_t i = 0; i < MAX_FRAME_LAG; i++) {
		VkDescriptorBufferInfo globalsBufferInfo{
			.buffer = uniformBuffers[i].getBuffer(),
			.offset = 0,
			.range = sizeof(GlobalUniforms)
		};

		VkDescriptorBufferInfo directionalBufferInfo{
			.buffer = scene.getDirectionalLightBuffer()->getBuffer(),
			.offset = 0,
			.range = sizeof(DirectionalLight)
		};

		VkDescriptorBufferInfo pointBufferInfo{
			.buffer = scene.getPointLightsBuffer()->getBuffer(),
			.offset = 0,
			.range = scene.getNumLights() * sizeof(PointLight)
		};

		std::vector<StorageBuffer*> clusterBuffers = clusterBuilder.getExternalBuffers();

		VkDescriptorBufferInfo clusterGenBufferInfo{
			.buffer = clusterBuffers[0]->getBuffer(),
			.offset = 0,
			.range = sizeof(ClusterGenData)
		};
		VkDescriptorBufferInfo lightIndicesBufferInfo{
			.buffer = clusterBuffers[1]->getBuffer(),
			.offset = 0,
			.range = clusterBuilder.getNumClusters() * clusterBuilder.getMaxLightsPerTile() * sizeof(uint32_t)
		};
		VkDescriptorBufferInfo lightGridBufferInfo{
			.buffer = clusterBuffers[2]->getBuffer(),
			.offset = 0,
			.range = clusterBuilder.getNumClusters() * 2 * sizeof(uint32_t)
		};

		std::array<VkWriteDescriptorSet, 6> writeDescriptorSets{};
		writeDescriptorSets[0] = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &globalsBufferInfo
		};
		writeDescriptorSets[1] = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.dstBinding = 1,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &directionalBufferInfo
		};
		writeDescriptorSets[2] = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.dstBinding = 2,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &pointBufferInfo
		};
		writeDescriptorSets[3] = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.dstBinding = 3,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &clusterGenBufferInfo
		};
		writeDescriptorSets[4] = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.dstBinding = 4,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &lightIndicesBufferInfo
		};
		writeDescriptorSets[5] = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.dstBinding = 5,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &lightGridBufferInfo
		};

		vkUpdateDescriptorSets(vulkanContext.device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
	}

	// Depth pre pass descriptor sets
	{
		std::vector<VkDescriptorSetLayout> depthLayouts(MAX_FRAME_LAG, depthPassDescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocateInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = descriptorPool,
			.descriptorSetCount = MAX_FRAME_LAG,
			.pSetLayouts = depthLayouts.data()
		};

		depthPassDescriptorSets.resize(MAX_FRAME_LAG);
		CHECK_VKRESULT(vkAllocateDescriptorSets(vulkanContext.device, &allocateInfo, depthPassDescriptorSets.data()));

		for (size_t i = 0; i < MAX_FRAME_LAG; i++) {
			VkDescriptorBufferInfo globalsBufferInfo{
				.buffer = uniformBuffers[i].getBuffer(),
				.offset = 0,
				.range = sizeof(GlobalUniforms)
			};

			VkWriteDescriptorSet writeDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = depthPassDescriptorSets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &globalsBufferInfo
			};

			vkUpdateDescriptorSets(vulkanContext.device, 1, &writeDescriptorSet, 0, nullptr);
		}
	}
}

void RenderingDevice::buildPrepassCommandBuffer() {
	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pInheritanceInfo = nullptr
	};

	std::array<VkClearValue, 1> clearValues{};
	clearValues[0].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo renderPassBeginInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = depthPrePass,
		.framebuffer = depthPrePassTarget.getFramebuffer(0),
		.renderArea = {
				.offset = { 0, 0 },
				.extent = {
						.width = vulkanContext.swapChain.width,
						.height = vulkanContext.swapChain.height } },
		.clearValueCount = clearValues.size(),
		.pClearValues = clearValues.data()
	};

	CHECK_VKRESULT(vkBeginCommandBuffer(depthPrePassCommandBuffers[frameIndex], &beginInfo));

	vkCmdBeginRenderPass(depthPrePassCommandBuffers[frameIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Update dynamic viewport state
	VkViewport viewport{
		.x = 0.f,
		.y = (float)vulkanContext.swapChain.height,
		.width = (float)vulkanContext.swapChain.width,
		.height = -((float)vulkanContext.swapChain.height),
		.minDepth = 0.f,
		.maxDepth = 1.f,
	};
	vkCmdSetViewport(depthPrePassCommandBuffers[frameIndex], 0, 1, &viewport);

	// Update dynamic scissor state
	VkRect2D scissor{
		.offset = { 0, 0 },
		.extent = { vulkanContext.swapChain.width, vulkanContext.swapChain.height }
	};
	vkCmdSetScissor(depthPrePassCommandBuffers[frameIndex], 0, 1, &scissor);

	vkCmdBindPipeline(depthPrePassCommandBuffers[frameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, depthPipeline);

	vkCmdBindDescriptorSets(depthPrePassCommandBuffers[frameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, depthPipelineLayout, 0, 1, &depthPassDescriptorSets[frameIndex], 0, nullptr);

	scene.draw(depthPrePassCommandBuffers[frameIndex], depthPipelineLayout, RenderFlag::None);

	vkCmdEndRenderPass(depthPrePassCommandBuffers[frameIndex]);
	CHECK_VKRESULT(vkEndCommandBuffer(depthPrePassCommandBuffers[frameIndex]));
}

void RenderingDevice::updateSwapchain(uint32_t* width, uint32_t* height) {
	vulkanContext.updateSwapchain(window);
	*width = vulkanContext.swapChain.width;
	*height = vulkanContext.swapChain.height;
}

}  // namespace vkw

}  // namespace bennu