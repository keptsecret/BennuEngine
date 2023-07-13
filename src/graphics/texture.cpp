#include <graphics/renderingdevice.h>
#include <graphics/texture.h>
#include <graphics/utilities.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace bennu {

namespace vkw {

void Texture::createImage(VkImage& image, VkDeviceMemory& memory, const VkExtent3D& extent, VkFormat format,
		VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, uint32_t mipLevels, uint32_t arrayLayers, VkImageType imageType) {
	RenderingDevice* rd = RenderingDevice::getSingleton();
	VkDevice device = rd->getDevice();

	VkImageCreateInfo imageCreateInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = imageType,
		.format = format,
		.extent = extent,
		.mipLevels = mipLevels,
		.arrayLayers = arrayLayers,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = tiling,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	CHECK_VKRESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &image));

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(device, image, &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = rd->getMemoryType(memoryRequirements.memoryTypeBits, properties)
	};

	CHECK_VKRESULT(vkAllocateMemory(device, &allocateInfo, nullptr, &memory));
	CHECK_VKRESULT(vkBindImageMemory(device, image, memory, 0));
}

void Texture::createImageView(VkImageView& imageView, VkImage const& image, VkImageViewType viewType, VkFormat format,
		VkImageAspectFlags imageAspect, uint32_t mipLevels, uint32_t baseMipLevel, uint32_t layerCount, uint32_t baseArrayLayer) {
	VkImageViewCreateInfo imageViewCreateInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = viewType,
		.format = format,
		.subresourceRange = {
				.aspectMask = imageAspect,
				.baseMipLevel = baseMipLevel,
				.levelCount = mipLevels,
				.baseArrayLayer = baseArrayLayer,
				.layerCount = layerCount }
	};

	VkDevice device = RenderingDevice::getSingleton()->getDevice();
	CHECK_VKRESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView));
}

void Texture::createImageSampler(VkSampler& sampler, VkFilter filter, VkSamplerAddressMode addressMode, bool anisotropic, uint32_t mipLevels) {
	RenderingDevice* rd = RenderingDevice::getSingleton();

	float maxAniso = (anisotropic && rd->getPhysicalDeviceFeatures().samplerAnisotropy)
			? rd->getPhysicalDeviceProperties().limits.maxSamplerAnisotropy
			: 1.f;

	VkSamplerCreateInfo samplerCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = filter,
		.minFilter = filter,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = addressMode,
		.addressModeV = addressMode,
		.addressModeW = addressMode,
		.mipLodBias = 0.f,
		.anisotropyEnable = anisotropic,
		.maxAnisotropy = maxAniso,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.f,
		.maxLod = 0.f,
		.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};

	CHECK_VKRESULT(vkCreateSampler(rd->getDevice(), &samplerCreateInfo, nullptr, &sampler));
}

void Texture::generateMipmaps(VkImage const& image, const VkExtent3D& extent, VkFormat format, VkImageLayout dstLayout,
		uint32_t mipLevels, uint32_t baseArrayLayer, uint32_t layerCount) {
	RenderingDevice* rd = RenderingDevice::getSingleton();

	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(rd->getPhysicalDevice(), format, &formatProperties);

	assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

	VkCommandBuffer commandBuffer;
	CHECK_VKRESULT(rd->createCommandBuffer(&commandBuffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true));

	VkImageMemoryBarrier barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.baseArrayLayer = baseArrayLayer,
				.layerCount = layerCount
		}
	};

	int32_t mipWidth = extent.width, mipHeight = extent.height;

	for (uint32_t i = 1; i < mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		VkImageBlit imageBlit{
			.srcSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = i - 1,
					.baseArrayLayer = baseArrayLayer,
					.layerCount = layerCount
			},
			.srcOffsets = {
					{ 0, 0, 0 },
					{ mipWidth, mipHeight, 1}
			},
			.dstSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = i,
					.baseArrayLayer = baseArrayLayer,
					.layerCount = layerCount
			},
			.dstOffsets = {
					{ 0, 0, 0 },
					{ mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1}
			}
		};
		vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = dstLayout;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		if (mipWidth > 1) {
			mipWidth /= 2;
		}
		if (mipHeight > 1) {
			mipHeight /= 2;
		}
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	barrier.newLayout = dstLayout;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	rd->commandBufferSubmitIdle(&commandBuffer, VK_QUEUE_GRAPHICS_BIT);
}

void Texture::transitionImageLayout(const VkImage& img, VkFormat format, VkImageLayout srcLayout, VkImageLayout dstLayout,
		VkImageAspectFlags imageAspect, uint32_t mipLevels, uint32_t baseMipLevel, uint32_t layerCount, uint32_t baseArrayLayer) {
	RenderingDevice* rd = RenderingDevice::getSingleton();
	VkCommandBuffer commandBuffer;
	CHECK_VKRESULT(rd->createCommandBuffer(&commandBuffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true));

	VkImageMemoryBarrier barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = srcLayout,
		.newLayout = dstLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = img,
		.subresourceRange = {
				.aspectMask = imageAspect,
				.baseMipLevel = baseMipLevel,
				.levelCount = mipLevels,
				.baseArrayLayer = baseArrayLayer,
				.layerCount = layerCount }
	};

	VkPipelineStageFlags srcStage, dstStage;

	if (srcLayout == VK_IMAGE_LAYOUT_UNDEFINED && dstLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (srcLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && dstLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (srcLayout == VK_IMAGE_LAYOUT_UNDEFINED && dstLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else {
		throw std::invalid_argument("ERROR::Texture:transitionImageLayout: unsupported layout transition!");
	}

	vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	rd->commandBufferSubmitIdle(&commandBuffer, VK_QUEUE_GRAPHICS_BIT);
}

void Texture::copyBufferToImage(const VkBuffer& buffer, const VkImage& img, const VkExtent3D& extent, uint32_t layerCount, uint32_t baseArrayLayer) {
	RenderingDevice* rd = RenderingDevice::getSingleton();
	VkCommandBuffer commandBuffer;
	CHECK_VKRESULT(rd->createCommandBuffer(&commandBuffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true));

	VkBufferImageCopy copyRegion{
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = baseArrayLayer,
				.layerCount = layerCount },
		.imageOffset = { 0, 0, 0 },
		.imageExtent = extent
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	rd->commandBufferSubmitIdle(&commandBuffer, VK_QUEUE_GRAPHICS_BIT);
}

uint32_t Texture::getMipLevels(const VkExtent3D& extent) {
	return (uint32_t)std::floor(std::log2(std::max(extent.width, std::max(extent.height, extent.depth)))) + 1;
}

VkFormat Texture::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (const auto& format : candidates) {
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(RenderingDevice::getSingleton()->getPhysicalDevice(), format, &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	return VK_FORMAT_UNDEFINED;
}

VkWriteDescriptorSet Texture::getWriteDescriptor(uint32_t binding, VkDescriptorType descriptorType) {
	VkDescriptorImageInfo imageInfo{
		.sampler = sampler,
		.imageView = imageView,
		.imageLayout = layout
	};

	VkWriteDescriptorSet writeDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = VK_NULL_HANDLE,
		.dstBinding = binding,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = descriptorType,
		.pImageInfo = &imageInfo
	};

	return writeDescriptorSet;
}

Texture::Texture(VkFormat format, VkImageLayout layout, uint32_t mipLevels, uint32_t arrayCount, VkFilter filter, VkSamplerAddressMode addressMode) :
		format(format), layout(layout), mipLevels(mipLevels), arrayCount(arrayCount), filter(filter), addressMode(addressMode) {}

Texture::~Texture() {
	VkDevice device = RenderingDevice::getSingleton()->getDevice();

	vkDestroySampler(device, sampler, nullptr);
	vkDestroyImageView(device, imageView, nullptr);
	vkFreeMemory(device, deviceMemory, nullptr);
	vkDestroyImage(device, image, nullptr);
}

bool Texture::hasDepth(VkFormat format) {
	static const std::vector<VkFormat> DEPTH_FORMATS = {
		VK_FORMAT_D16_UNORM, VK_FORMAT_X8_D24_UNORM_PACK32, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT
	};
	return std::find(DEPTH_FORMATS.begin(), DEPTH_FORMATS.end(), format) != std::end(DEPTH_FORMATS);
}

bool Texture::hasStencil(VkFormat format) {
	static const std::vector<VkFormat> STENCIL_FORMATS = {
		VK_FORMAT_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT
	};
	return std::find(STENCIL_FORMATS.begin(), STENCIL_FORMATS.end(), format) != std::end(STENCIL_FORMATS);
}

Texture2D::Texture2D(VkFormat format, VkImageLayout layout, VkFilter filter, VkSamplerAddressMode addressMode, bool aniso, bool mipmap) :
		Texture(format, layout, 1, 1, filter, addressMode), anisotropic(aniso), mipmap(mipmap) {
}

void Texture2D::loadFromFile(const std::string& filename, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) {
	int width, height, channels;
	unsigned char* pixels = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	if (!pixels) {
		std::cerr << "ERROR::Texture2D:loadTextureFromFile(): Texture failed to load at path: " << filename << '\n';
		if (stbi_failure_reason()) {
			std::cerr << stbi_failure_reason();
		}
		return;
	}

	VkDeviceSize textureSize = width * height * 4;
	extent = { (uint32_t)width, (uint32_t)height, 1 };

	mipLevels = mipmap ? getMipLevels(extent) : 1;

	Buffer stagingBuffer(textureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pixels);
	stbi_image_free(pixels);

	this->usage = usage;

	createImage(image, deviceMemory, extent, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mipLevels, arrayCount, VK_IMAGE_TYPE_2D);
	createImageSampler(sampler, filter, addressMode, anisotropic, mipLevels);
	createImageView(imageView, image, VK_IMAGE_VIEW_TYPE_2D, format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayCount, 0);

	transitionImageLayout(image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayCount, 0);
	copyBufferToImage(stagingBuffer.getBuffer(), image, extent, 1, 0);

	if (mipmap) {
		generateMipmaps(image, extent, format, layout, mipLevels, 0, arrayCount);
	} else {
		transitionImageLayout(image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayCount, 0);
	}
}

// depth formats in order of importance
static const std::vector<VkFormat> DEPTH_FORMATS = {
	VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT,
	VK_FORMAT_D16_UNORM
};

TextureDepth::TextureDepth(const glm::ivec2& extent, VkSampleCountFlagBits samples) :
		Texture(findSupportedFormat(DEPTH_FORMATS, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				1, 1, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE) {
	this->extent = { (uint32_t)extent.x, (uint32_t)extent.y, 1 };

	if (format == VK_FORMAT_UNDEFINED) {
		throw std::runtime_error("ERROR::TextureDepth:TextureDepth: failed to find supported depth format!");
	}

	VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	if (hasStencil(format)) {
		aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	createImage(image, deviceMemory, this->extent, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, 1, VK_IMAGE_TYPE_2D);
	createImageSampler(sampler, filter, addressMode, false, mipLevels);
	createImageView(imageView, image, VK_IMAGE_VIEW_TYPE_2D, format, VK_IMAGE_ASPECT_DEPTH_BIT, 1, 0, 1, 0);
	transitionImageLayout(image, format, VK_IMAGE_LAYOUT_UNDEFINED, layout, aspectMask, 1, 0, 1, 0);
}

}  // namespace vkw

}  // namespace bennu