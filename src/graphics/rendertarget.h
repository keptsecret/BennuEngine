#ifndef BENNU_RENDERTARGET_H
#define BENNU_RENDERTARGET_H

#include <graphics/texture.h>
#include <graphics/swapchain.h>

#include <memory>

namespace bennu {

namespace vkw {

struct AttachmentInfo {
	AttachmentInfo(Texture* texture) :
			format(texture->getFormat()),
			isSwapchainResource(false),
			texture(texture) {}

	AttachmentInfo(Swapchain* swapchain) :
			format(swapchain->getFormat()),
			swapchain(swapchain),
			isSwapchainResource(true) {}

	VkFormat format;
	VkAttachmentLoadOp loadAction = VK_ATTACHMENT_LOAD_OP_CLEAR;
	VkAttachmentStoreOp storeAction = VK_ATTACHMENT_STORE_OP_STORE;

	bool isSwapchainResource;
	Texture* texture = nullptr;
	Swapchain* swapchain = nullptr;
};

class RenderTarget {
public:
	RenderTarget() {}

	void addColorAttachment(AttachmentInfo& attachment);
	void addColorResolveAttachment(AttachmentInfo& attachment);
	void setDepthStencilAttachment(AttachmentInfo& attachment);

	void setupFramebuffers(uint32_t count, VkExtent2D extent, VkRenderPass renderPass);
	void destroy();

	uint32_t getNumColorAttachments() const { return colorReferences.size(); }
	bool getHasResolveAttachments() const { return hasResolveAttachments; }
	bool getHasDepthStencil() const { return hasDepthStencil; }

	const VkAttachmentReference* getColorAttachmentReferences() const { return colorReferences.data(); }
	const VkAttachmentReference* getResolveAttachmentReferences() const { return hasResolveAttachments ? resolveReferences.data() : nullptr; }
	const VkAttachmentReference* getDepthStencilReference() const { return hasDepthStencil ? &depthStencilReference : nullptr; }

	uint32_t getNumAttachmentDescriptions() const { return descriptions.size(); }
	const VkAttachmentDescription* getAttachmentDescriptions() const { return descriptions.data(); }

	const VkFramebuffer& getFramebuffer(int index) const { return framebuffers[index]; }

private:
	std::vector<VkFramebuffer> framebuffers;
	VkExtent2D extent;

	uint32_t numAttachments = 0;
	// Unused, might need?
//	std::vector<VkImage> colorRenderTargets;
//	std::vector<VkImage> colorResolveTargets;
//	VkImage depthStencilTarget;

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

#endif	// BENNU_RENDERTARGET_H
