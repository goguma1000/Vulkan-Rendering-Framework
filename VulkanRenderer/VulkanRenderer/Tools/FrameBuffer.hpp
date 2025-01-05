#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

extern const int MAX_FRAMES_IN_FLIGHT;
class FrameBuffer {
	std::vector<VkFramebuffer> framebuffers;
public:
	void Clean();
	void SetUp(VkExtent2D extent, std::vector<VkImageView>& attachments, VkRenderPass renderPass, const uint32_t frameCount = MAX_FRAMES_IN_FLIGHT);
	const VkFramebuffer GetCurrentFrameBuffer(int curFrame) const { return framebuffers[curFrame]; }
};
namespace Utils {
	void CreateFrameBuffer(VkFramebuffer& out, const VkDevice device, const std::vector<VkImageView>& attachments, const VkRenderPass renderpass, const VkExtent2D& extent);
}