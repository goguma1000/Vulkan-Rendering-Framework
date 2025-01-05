#include <Tools/FrameBuffer.hpp>
#include <Tools/Utils.hpp>
#include "Renderer.h"

void FrameBuffer::Clean() {
	Renderer* instance = Renderer::GetInstance();
	for (size_t i = 0; i < framebuffers.size(); i++) {
		vkDestroyFramebuffer(instance->device, framebuffers[i], nullptr);
	}
}
void FrameBuffer::SetUp(VkExtent2D extent, std::vector<VkImageView>& attachments, VkRenderPass renderPass, const uint32_t frameCount) {
	Renderer* instnace = Renderer::GetInstance();
	framebuffers.resize(frameCount);
	for (uint32_t i = 0; i < frameCount; i++) {
		Utils::CreateFrameBuffer(framebuffers[i], instnace->device, attachments, renderPass, extent);
	}
}

namespace Utils {
	void Utils::CreateFrameBuffer(VkFramebuffer& out, const VkDevice device, const std::vector<VkImageView>& attachments, const VkRenderPass renderpass, const VkExtent2D& extent) {
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderpass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &out) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}