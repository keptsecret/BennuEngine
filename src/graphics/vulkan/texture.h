#ifndef BENNU_TEXTURE_H
#define BENNU_TEXTURE_H

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace bennu {

namespace vkw {

class Texture {
public:
	~Texture();

	const VkImage& getImage() const { return image; }
	const VkImageView& getImageView() const { return imageView; }
	const VkSampler& getSampler() const { return sampler; }
	VkImageLayout getLayout() const { return layout; }

	VkFormat getFormat() const { return format; }
	VkSampleCountFlagBits getSamples() const { return samples; }

	static bool hasDepth(VkFormat format);
	static bool hasStencil(VkFormat format);

	// TODO: figure out what to do wrt descriptors
	VkWriteDescriptorSet getWriteDescriptor(uint32_t binding, VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	static void createImage(VkImage& image, VkDeviceMemory& memory, const VkExtent3D& extent, VkFormat format, VkSampleCountFlagBits samples,
			VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, uint32_t mipLevels, uint32_t arrayLayers, VkImageType imageType);
	static void createImageView(VkImageView& imageView, const VkImage& image, VkImageViewType type, VkFormat format,
			VkImageAspectFlags imageAspect, uint32_t mipLevels, uint32_t baseMipLevel, uint32_t layerCount, uint32_t baseArrayLayer);
	static void createImageSampler(VkSampler& sampler, VkFilter filter, VkSamplerAddressMode addressMode, bool anisotropic, uint32_t mipLevels);

	static void generateMipmaps(const VkImage& image, const VkExtent3D& extent, VkFormat format, VkImageLayout dstLayout,
			uint32_t mipLevels, uint32_t baseArrayLayer, uint32_t layerCount);
	static void transitionImageLayout(const VkImage& image, VkFormat format, VkImageLayout srcLayout, VkImageLayout dstLayout,
			VkImageAspectFlags imageAspect, uint32_t mipLevels, uint32_t baseMipLevel, uint32_t layerCount, uint32_t baseArrayLayer);
	static void copyBufferToImage(const VkBuffer& buffer, const VkImage& image, const VkExtent3D& extent, uint32_t layerCount, uint32_t baseArrayLayer);

	static uint32_t getMipLevels(const VkExtent3D& extent);
	static VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

protected:
	Texture(VkFormat format, VkImageLayout layout, VkImageUsageFlags usage, VkFilter filter, VkSamplerAddressMode addressMode, VkSampleCountFlagBits samples, uint32_t mipLevels, uint32_t arrayCount);

	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkSampler sampler = VK_NULL_HANDLE;

	VkFilter filter;
	VkImageLayout layout;
	VkSamplerAddressMode addressMode;
	VkSampleCountFlagBits samples;

	VkExtent3D extent;
	VkImageUsageFlags usage;
	VkFormat format = VK_FORMAT_UNDEFINED;
	uint32_t mipLevels = 0;
	uint32_t arrayCount;
};

class Texture2D : public Texture {
public:
	Texture2D(const std::string& filename, VkFilter filter = VK_FILTER_LINEAR, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			bool aniso = true, bool mipmap = true);

	Texture2D(const glm::ivec2& extent, const void* pixels = nullptr, VkDeviceSize bufferSize = 0,
			VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VkFilter filter = VK_FILTER_LINEAR, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT, bool aniso = false, bool mipmap = false);

private:
	void initialize();
	void loadFromFile(const std::string& filename);

	bool anisotropic;
	bool mipmap;
};

class TextureDepth : public Texture {
public:
	TextureDepth(const glm::ivec2& extent, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
};

}  // namespace vkw

}  // namespace bennu

#endif	// BENNU_TEXTURE_H
