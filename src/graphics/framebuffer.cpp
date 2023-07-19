#include <graphics/framebuffer.h>
#include <graphics/renderingdevice.h>
#include <graphics/utilities.h>

namespace bennu {

namespace vkw {

void RenderTarget::addColorAttachment(AttachmentInfo& attachment) {
	attachments.push_back(attachment);
	colorRenderTargets.push_back(attachment.image);

	VkAttachmentReference reference{
		.attachment = numAttachments,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	colorReferences.push_back(reference);

	VkAttachmentDescription description{
		.format = attachment.texture->getFormat(),
		.samples = attachment.texture->getSamples(),
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	descriptions.push_back(description);

	numAttachments++;
}

void RenderTarget::addColorResolveAttachment(AttachmentInfo& attachment) {
	attachments.push_back(attachment);
	colorResolveTargets.push_back(attachment.image);

	VkAttachmentReference reference{
		.attachment = numAttachments,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	resolveReferences.push_back(reference);

	VkAttachmentDescription description{
		.format = attachment.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	descriptions.push_back(description);

	hasResolveAttachments = true;
	numAttachments++;
}

void RenderTarget::setDepthStencilAttachment(AttachmentInfo& attachment) {
	attachments.push_back(attachment);
	depthStencilTarget = attachment.image;

	VkAttachmentReference reference{
		.attachment = numAttachments,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	depthStencilReference = reference;

	VkAttachmentDescription description{
		.format = attachment.texture->getFormat(),
		.samples = attachment.texture->getSamples(),
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	descriptions.push_back(description);

	hasDepthStencil = true;
	numAttachments++;
}

void RenderTarget::setupFramebuffer(VkExtent2D ext, VkRenderPass renderPass) {
	extent = ext;

	std::vector<VkImageView> viewAttachments(attachments.size());
	for (uint32_t i = 0; i < attachments.size(); i++) {
		viewAttachments[i] = attachments[i].imageView;
	}

	VkFramebufferCreateInfo framebufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = renderPass,
		.attachmentCount = (uint32_t)viewAttachments.size(),
		.pAttachments = viewAttachments.data(),
		.width = extent.width,
		.height = extent.height,
		.layers = 1
	};

	CHECK_VKRESULT(vkCreateFramebuffer(RenderingDevice::getSingleton()->getDevice(), &framebufferCreateInfo, nullptr, &framebuffer));
}

void RenderTarget::destroy() {
	while (!attachments.empty()) {
		AttachmentInfo attachment = attachments.back();

		if (!attachment.isSwapchainResource && attachment.texture) {
			delete attachment.texture;
		}

		attachments.pop_back();
	}

	vkDestroyFramebuffer(RenderingDevice::getSingleton()->getDevice(), framebuffer, nullptr);
	numAttachments = 0;
}

}  // namespace vkw

}  // namespace bennu