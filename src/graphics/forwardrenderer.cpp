#include <graphics/forwardrenderer.h>
#include <graphics/rendersystem.h>
#include <graphics/vulkan/renderingdevice.h>
#include <graphics/vulkan/utilities.h>

#include <array>

namespace bennu {

void ForwardRenderer::setupDescriptorSetLayouts() {
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

	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();

	VkDescriptorSetLayout globalSetLayout;
	CHECK_VKRESULT(vkCreateDescriptorSetLayout(rd->getDevice(), &layoutCreateInfo, nullptr, &globalSetLayout));
	descriptorSetLayouts.push_back(globalSetLayout);

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
	CHECK_VKRESULT(vkCreateDescriptorSetLayout(rd->getDevice(), &layoutCreateInfo, nullptr, &imageSetLayout));
	descriptorSetLayouts.push_back(imageSetLayout);
}

void ForwardRenderer::createRenderPipelines(std::vector<VkPipelineShaderStageCreateInfo>& shaderStages) {
	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();

	//	VkPipelineShaderStageCreateInfo shaderStages[] = {
	//		loadSPIRVShader("../src/graphics/shaders/forward.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
	//		loadSPIRVShader("../src/graphics/shaders/forward.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
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

	CHECK_VKRESULT(vkCreatePipelineLayout(rd->getDevice(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = (uint32_t)shaderStages.size(),
		.pStages = shaderStages.data(),
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

	CHECK_VKRESULT(vkCreateGraphicsPipelines(rd->getDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline));
}

void ForwardRenderer::setupRenderPass() {
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

	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();
	CHECK_VKRESULT(vkCreateRenderPass(rd->getDevice(), &renderPassCreateInfo, nullptr, &renderPass));
}

void ForwardRenderer::setupTargetFramebuffers(uint32_t count, uint32_t width, uint32_t height) {
	renderTarget.setupFramebuffers(count, { width, height }, renderPass);
}

void ForwardRenderer::cleanupRenderSurface() {
	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();
	renderTarget.destroy();
	vkDestroyRenderPass(rd->getDevice(), renderPass, nullptr);
}

void ForwardRenderer::createCommandBuffers() {
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

void ForwardRenderer::createDescriptorSets(const Scene& scene, std::vector<ForwardRendererBuffersInfo>& buffersInfo) {
	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();
	RenderSystem* rs = RenderSystem::getSingleton();
	uint32_t setCount = rs->getMaxFrameLag();
	// buffersInfo should be the same size as setCount

	std::vector<VkDescriptorSetLayout> layouts(setCount, descriptorSetLayouts[0]);
	VkDescriptorSetAllocateInfo allocateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = rd->getDescriptorPool(),
		.descriptorSetCount = setCount,
		.pSetLayouts = layouts.data()
	};

	descriptorSets.resize(setCount);
	CHECK_VKRESULT(vkAllocateDescriptorSets(rd->getDevice(), &allocateInfo, descriptorSets.data()));

	for (size_t i = 0; i < setCount; i++) {
		VkDescriptorBufferInfo globalsBufferInfo{
			.buffer = buffersInfo[i].uniforms->getBuffer(),
			.offset = 0,
			.range = buffersInfo[i].uniformSize
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

		VkDescriptorBufferInfo clusterGenBufferInfo{
			.buffer = buffersInfo[i].clusterGen->getBuffer(),
			.offset = 0,
			.range = buffersInfo[i].clusterGenSize
		};
		VkDescriptorBufferInfo lightIndicesBufferInfo{
			.buffer = buffersInfo[i].lightIndices->getBuffer(),
			.offset = 0,
			.range = buffersInfo[i].lightIndicesSize
		};
		VkDescriptorBufferInfo lightGridBufferInfo{
			.buffer = buffersInfo[i].lightGrid->getBuffer(),
			.offset = 0,
			.range = buffersInfo[i].lightGridSize
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

		vkUpdateDescriptorSets(rd->getDevice(), writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
	}
}

}  // namespace bennu