#include <graphics/depthrenderer.h>

#include <graphics/rendersystem.h>
#include <graphics/vulkan/renderingdevice.h>
#include <graphics/vulkan/utilities.h>

namespace bennu {

void DepthRenderer::setupDescriptorSetLayouts() {
	VkDescriptorSetLayoutBinding globalsLayoutBinding{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.pImmutableSamplers = nullptr
	};

	VkDescriptorSetLayoutCreateInfo prepassLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &globalsLayoutBinding
	};

	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();
	CHECK_VKRESULT(vkCreateDescriptorSetLayout(rd->getDevice(), &prepassLayoutCreateInfo, nullptr, &descriptorSetLayout));
}

void DepthRenderer::createRenderPipelines(std::vector<VkPipelineShaderStageCreateInfo>& shaderStages) {
	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();

//	VkPipelineShaderStageCreateInfo prepassShaderStages[] = {
//		loadSPIRVShader("../src/graphics/shaders/depth.vert.spv", VK_SHADER_STAGE_VERTEX_BIT)
//	};

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
		.rasterizationSamples = rd->getMSAASamples(),
		.sampleShadingEnable = VK_FALSE
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

	VkPushConstantRange pushConstants{
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof(MeshPushConstants)
	};

	VkPipelineLayoutCreateInfo depthPrePassPipelineLayoutCreateInfo{ ///< good idea to separate this out
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &descriptorSetLayout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstants
	};

	CHECK_VKRESULT(vkCreatePipelineLayout(rd->getDevice(), &depthPrePassPipelineLayoutCreateInfo, nullptr, &pipelineLayout));

	VkGraphicsPipelineCreateInfo prepassPipelineCreateInfo{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = (uint32_t)shaderStages.size(),
		.pStages = shaderStages.data(),
		.pVertexInputState = &vertexInputState,
		.pInputAssemblyState = &inputAssemblyState,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizationState,
		.pMultisampleState = &multisampleState,
		.pDepthStencilState = &prepassDepthStencilState,
		.pColorBlendState = nullptr,
		.pDynamicState = &dynamicState,
		.layout = pipelineLayout,
		.renderPass = renderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	CHECK_VKRESULT(vkCreateGraphicsPipelines(rd->getDevice(), VK_NULL_HANDLE, 1, &prepassPipelineCreateInfo, nullptr, &pipeline));
}

void DepthRenderer::setupRenderPass() {
	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();

	VkSubpassDescription subpass{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 0,
		.pDepthStencilAttachment = renderTarget.getDepthStencilReference()
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
		.attachmentCount = renderTarget.getNumAttachmentDescriptions(),
		.pAttachments = renderTarget.getAttachmentDescriptions(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};

	CHECK_VKRESULT(vkCreateRenderPass(rd->getDevice(), &renderPassCreateInfo, nullptr, &renderPass));
}

void DepthRenderer::setupTargetFramebuffers(uint32_t count, uint32_t width, uint32_t height) {
	renderTarget.setupFramebuffers(count, {width, height}, renderPass);
}

void DepthRenderer::cleanupRenderSurface() {
	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();
	renderTarget.destroy();
	vkDestroyRenderPass(rd->getDevice(), renderPass, nullptr);
}

void DepthRenderer::createCommandBuffers() {
	uint32_t commandBufferCount = RenderSystem::getSingleton()->getMaxFrameLag();
	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();

	VkCommandBufferAllocateInfo allocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = rd->getCommandPool(),
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = commandBufferCount
	};

	commandBuffers.resize(commandBufferCount);
	CHECK_VKRESULT(vkAllocateCommandBuffers(rd->getDevice(), &allocateInfo, commandBuffers.data()));
}

void DepthRenderer::createDescriptorSets(const Scene& scene, std::vector<DepthRendererBuffersInfo> buffersInfo) {
	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();
	RenderSystem* rs = RenderSystem::getSingleton();
	uint32_t setCount = rs->getMaxFrameLag();
	// buffersInfo should be the same size as setCount

	std::vector<VkDescriptorSetLayout> depthLayouts(setCount, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = rd->getDescriptorPool(),
		.descriptorSetCount = setCount,
		.pSetLayouts = depthLayouts.data()
	};

	descriptorSets.resize(setCount);
	CHECK_VKRESULT(vkAllocateDescriptorSets(rd->getDevice(), &allocateInfo, descriptorSets.data()));

	for (size_t i = 0; i < setCount; i++) {
		VkDescriptorBufferInfo globalsBufferInfo{
			.buffer = buffersInfo[i].uniforms->getBuffer(),
			.offset = 0,
			.range = buffersInfo[i].uniformSize
		};

		VkWriteDescriptorSet writeDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &globalsBufferInfo
		};

		vkUpdateDescriptorSets(rd->getDevice(), 1, &writeDescriptorSet, 0, nullptr);
	}
}

}  // namespace bennu