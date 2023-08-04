#include <graphics/renderingdevice.h>
#include <graphics/utilities.h>
#include <scene/material.h>

namespace bennu {

Material::Material() :
		auxUbo(sizeof(MaterialAux)) {
}

void Material::apply() {
	// Fills in missing textures with constant values (1x1 textures)
	if (!albedoTexture) {
		std::shared_ptr<Texture> texture = std::make_shared<Texture>();
		texture->isConstantValue = true;
		texture->constant = glm::vec4{ albedo, 1 };

		///< 3-component on Nvidia drivers seems not supported
		vkw::Texture2D* albedoConst = new vkw::Texture2D({ 1, 1 }, glm::value_ptr(texture->constant), 4 * sizeof(float),
				VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FILTER_NEAREST,
				VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLE_COUNT_1_BIT);
		texture->texture = std::unique_ptr<vkw::Texture>(albedoConst);

		albedoTexture = texture;
	}

	if (!metallicTexture) {
		std::shared_ptr<Texture> texture = std::make_shared<Texture>();
		texture->isConstantValue = true;
		texture->constant = glm::vec4{ metallic };

		vkw::Texture2D* metallicConst = new vkw::Texture2D({ 1, 1 }, &metallic, sizeof(float),
				VK_FORMAT_R32_SFLOAT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FILTER_NEAREST,
				VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLE_COUNT_1_BIT);
		texture->texture = std::unique_ptr<vkw::Texture>(metallicConst);

		metallicTexture = texture;
	}

	if (!roughnessTexture) {
		std::shared_ptr<Texture> texture = std::make_shared<Texture>();
		texture->isConstantValue = true;
		texture->constant = glm::vec4{ roughness };

		vkw::Texture2D* roughConst = new vkw::Texture2D({ 1, 1 }, &roughness, sizeof(float),
				VK_FORMAT_R32_SFLOAT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FILTER_NEAREST,
				VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLE_COUNT_1_BIT);
		texture->texture = std::unique_ptr<vkw::Texture>(roughConst);

		roughnessTexture = texture;
	}

	if (!ambientTexture) {
		std::shared_ptr<Texture> texture = std::make_shared<Texture>();
		texture->isConstantValue = true;
		texture->constant = glm::vec4{ ambient };

		vkw::Texture2D* aoConst = new vkw::Texture2D({ 1, 1 }, &ambient, sizeof(float),
				VK_FORMAT_R32_SFLOAT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FILTER_NEAREST,
				VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLE_COUNT_1_BIT);
		texture->texture = std::unique_ptr<vkw::Texture>(aoConst);

		ambientTexture = texture;
	}

	if (!normalMap) {
		std::shared_ptr<Texture> texture = std::make_shared<Texture>();
		texture->isConstantValue = true;
		texture->constant = glm::vec4{ 0 };

		float zero = 0;
		vkw::Texture2D* normalDummy = new vkw::Texture2D({ 1, 1 }, &zero, sizeof(float),
				VK_FORMAT_R32_SFLOAT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FILTER_NEAREST,
				VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLE_COUNT_1_BIT);
		texture->texture = std::unique_ptr<vkw::Texture>(normalDummy);

		normalMap = texture;
	}

	auxUbo.update(&aux);
}

void Material::createDescriptorSet(VkDescriptorPool const& descriptorPool, VkDescriptorSetLayout const& descriptorSetLayout, uint32_t descriptorBindingFlags) {
	VkDevice device = vkw::RenderingDevice::getSingleton()->getDevice();

	VkDescriptorSetAllocateInfo allocateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &descriptorSetLayout
	};
	CHECK_VKRESULT(vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSet));

	std::vector<VkWriteDescriptorSet> writeDescriptorSets{};
	VkDescriptorBufferInfo bufferInfo{
		.buffer = auxUbo.getBuffer(),
		.offset = 0,
		.range = sizeof(MaterialAux)
	};
	VkWriteDescriptorSet bufWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptorSet,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo = &bufferInfo
	};
	writeDescriptorSets.push_back(bufWriteDescriptorSet);

	VkDescriptorImageInfo albedoImageInfo{
		.sampler = albedoTexture->texture->getSampler(),
		.imageView = albedoTexture->texture->getImageView(),
		.imageLayout = albedoTexture->texture->getLayout()
	};
	VkWriteDescriptorSet albedoWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptorSet,
		.dstBinding = (uint32_t)writeDescriptorSets.size(),
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &albedoImageInfo
	};
	writeDescriptorSets.push_back(albedoWriteDescriptorSet);

	VkDescriptorImageInfo metallicImageInfo{
		.sampler = metallicTexture->texture->getSampler(),
		.imageView = metallicTexture->texture->getImageView(),
		.imageLayout = metallicTexture->texture->getLayout()
	};
	VkWriteDescriptorSet metallicWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptorSet,
		.dstBinding = (uint32_t)writeDescriptorSets.size(),
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &metallicImageInfo
	};
	writeDescriptorSets.push_back(metallicWriteDescriptorSet);

	VkDescriptorImageInfo roughnessImageInfo{
		.sampler = roughnessTexture->texture->getSampler(),
		.imageView = roughnessTexture->texture->getImageView(),
		.imageLayout = roughnessTexture->texture->getLayout()
	};
	VkWriteDescriptorSet roughnessWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptorSet,
		.dstBinding = (uint32_t)writeDescriptorSets.size(),
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &roughnessImageInfo
	};
	writeDescriptorSets.push_back(roughnessWriteDescriptorSet);

	VkDescriptorImageInfo ambientImageInfo{
		.sampler = ambientTexture->texture->getSampler(),
		.imageView = ambientTexture->texture->getImageView(),
		.imageLayout = ambientTexture->texture->getLayout()
	};
	VkWriteDescriptorSet ambientWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptorSet,
		.dstBinding = (uint32_t)writeDescriptorSets.size(),
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &ambientImageInfo
	};
	writeDescriptorSets.push_back(ambientWriteDescriptorSet);

	VkDescriptorImageInfo normalsImageInfo{
		.sampler = normalMap->texture->getSampler(),
		.imageView = normalMap->texture->getImageView(),
		.imageLayout = normalMap->texture->getLayout()
	};
	VkWriteDescriptorSet normalsWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptorSet,
		.dstBinding = (uint32_t)writeDescriptorSets.size(),
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &normalsImageInfo
	};
	writeDescriptorSets.push_back(normalsWriteDescriptorSet);

	vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
}

Material::~Material() {
	if (albedoTexture && albedoTexture->isConstantValue) {
		albedoTexture->texture.reset();
	}

	if (metallicTexture && metallicTexture->isConstantValue) {
		metallicTexture->texture.reset();
	}

	if (roughnessTexture && roughnessTexture->isConstantValue) {
		roughnessTexture->texture.reset();
	}

	if (ambientTexture && ambientTexture->isConstantValue) {
		ambientTexture->texture.reset();
	}

	if (normalMap && normalMap->isConstantValue) {
		normalMap->texture.reset();
	}
}

}  // namespace bennu