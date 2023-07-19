#ifndef BENNU_FRAMEBUFFER_H
#define BENNU_FRAMEBUFFER_H

#include <graphics/texture.h>

#include <memory>

namespace bennu {

namespace vkw {

struct AttachmentInfo {
	AttachmentInfo(Texture* texture) :
			image(texture->getImage()),
			imageView(texture->getImageView()),
			format(texture->getFormat()),
			layout(texture->getLayout()),
			isSwapchainResource(false),
			texture(texture) {}

	AttachmentInfo(VkImage image, VkImageView imageView, VkFormat format) :
			image(image), imageView(imageView),
			format(format),
			isSwapchainResource(true) {}

	VkImage image = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;

	VkFormat format;
	VkImageLayout layout;

	bool isSwapchainResource;
	Texture* texture = nullptr;
};

class RenderTarget {
public:
	RenderTarget() {}

	void addColorAttachment(AttachmentInfo& attachment);
	void addColorResolveAttachment(AttachmentInfo& attachment);
	void setDepthStencilAttachment(AttachmentInfo& attachment);

	void setupFramebuffer(VkExtent2D extent, VkRenderPass renderPass);
	void destroy();

	uint32_t getNumColorAttachments() const { return colorReferences.size(); }
	bool getHasResolveAttachments() const { return hasResolveAttachments; }
	bool getHasDepthStencil() const { return hasDepthStencil; }

	const VkAttachmentReference* getColorAttachmentReferences() const { return colorReferences.data(); }
	const VkAttachmentReference* getResolveAttachmentReferences() const { return resolveReferences.data(); }
	const VkAttachmentReference* getDepthStencilReference() const { return &depthStencilReference; }

	uint32_t getNumAttachmentDescriptions() const { return descriptions.size(); }
	const VkAttachmentDescription* getAttachmentDescriptions() const { return descriptions.data(); }

	const VkFramebuffer& getFramebuffer() const { return framebuffer; }

private:
	VkFramebuffer framebuffer = VK_NULL_HANDLE;
	VkExtent2D extent;

	uint32_t numAttachments = 0;
	std::vector<VkImage> colorRenderTargets;
	std::vector<VkImage> colorResolveTargets;
	VkImage depthStencilTarget;

	std::vector<VkAttachmentReference> colorReferences;
	std::vector<VkAttachmentReference> resolveReferences;
	VkAttachmentReference depthStencilReference;
	bool hasResolveAttachments = false;
	bool hasDepthStencil = false;

	//	clearcolor

	std::vector<VkAttachmentDescription> descriptions;

	std::vector<AttachmentInfo> attachments;
};

}  // namespace vkw

}  // namespace bennu

#endif	// BENNU_FRAMEBUFFER_H
