#pragma once
#ifndef UTILS_Hpp
#define UTILS_Hpp

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include<iostream>
#include<vector>
#include<optional>
#include<cstring>
#include<filesystem>
#include<string>


namespace Utils {

	struct QueueFamilyIndices {
		//std::optional is a wrapper that contains no value until you assign something to it.
		//supported in after c++17
		std::optional<uint32_t> graphicsFamily; //write 2024 - 08 - 15__03:10
		std::optional<uint32_t> presentFamily;  //write 2024 - 08 - 15__03:56
		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {
		if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			//Message is important enough to show
		}
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}

	std::vector<const char*> GetRequiredExtension(bool enableValidationLayer);
	bool CheckValidationLayerSupport(const std::vector<const char*>& validationLayers);
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,const VkAllocationCallbacks* pAllocator,VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance,VkDebugUtilsMessengerEXT debugMessenger,const VkAllocationCallbacks* pAllocator);
	QueueFamilyIndices FindQueueFamiles(VkPhysicalDevice device, VkSurfaceKHR surface);
	SwapChainSupportDetails QuerrySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
	VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageViewType viewType, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1);
	VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);
	uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory,VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE, VkDeviceSize memoffset = 0);
	void CopyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue submitQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize _size);
	void CreateVertexUniforBuffer(VkDevice device,VkPhysicalDevice physicalDevice, std::vector<VkBuffer>& buffer, std::vector<VkDeviceMemory>& bufferMemory, std::vector<void*>& mappedMemory, uint32_t bufferCount = 2);
	void CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, VkImage& image, VkDeviceMemory& imageMemory, VkMemoryPropertyFlags properties, VkImageCreateInfo& imageInfo, VkDeviceSize memoryOffset = 0);
	VkCommandBuffer BeginSingleTimeCommand(VkDevice device, VkCommandPool commandPool);
	void EndSingleTimeCommand(VkDevice device, VkCommandPool commandPool, VkQueue submitQueue, VkCommandBuffer commandBuffer);
	void transitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue submitQueue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,uint32_t mipLevels);
	std::string getPath(const std::string& filename);
}

namespace Initializer {

	VkSemaphoreCreateInfo InitSemaphoreCreateInfo(void* next = NULL, VkSemaphoreCreateFlags flag = 0);
	VkFenceCreateInfo InitFenceCreateInfo(VkFenceCreateFlagBits flag, void* next = NULL);
	VkSubmitInfo InitSubmitInfo(uint32_t _waitSemaphoreCount, VkSemaphore* _pWaitSemaphores, VkPipelineStageFlags* _pWaitDstStageMask, uint32_t _commandBufferCount, VkCommandBuffer* _pCommandBuffers, uint32_t _signalSemaphoreCount, VkSemaphore* _pSignalSemaphores);
	VkPresentInfoKHR InitPresentInfo(uint32_t _waitSemaphoreCount, VkSemaphore* _pWaitSemaphores, uint32_t _swapChainCount, VkSwapchainKHR* _pSwapChains, uint32_t* _pImageIndices, VkResult* _pResult = nullptr);
	VkCommandBufferBeginInfo InitCommandBufferBeginInfo(VkCommandBufferUsageFlags _flags = 0, VkCommandBufferInheritanceInfo* _pInheritanceInfo = nullptr, void* _pNext = nullptr);
	VkRenderPassBeginInfo InitRenderPassBeginInfo(VkRenderPass _renderPass, VkFramebuffer _framebuffer, VkOffset2D _offset, VkExtent2D swapChainExtent, uint32_t _clearValueCount, VkClearValue* _pClearValues);
	VkViewport InitViewport(float _x, float _y, float _width, float _height, float _minDepth, float _maxDepth);
	VkRect2D InitScissor(VkOffset2D _offset, VkExtent2D _extent);
	VkBufferCreateInfo InitBufferCreateInfo(VkDeviceSize _size, VkBufferUsageFlags _usage, VkSharingMode _sharingMode);
	VkMemoryAllocateInfo InitMemoryAllocateInfo(VkDeviceSize _allocationSize, uint32_t _memoryTypeIndex);
	VkImageCreateInfo InitImageCreateInfo(VkImageType _imageType, uint32_t _width, uint32_t _height, uint32_t _depth, uint32_t _miplevels,VkFormat _format, VkImageTiling _tiling, VkImageUsageFlags _usage, VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT, VkImageLayout _initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, VkSharingMode _sharingMode = VK_SHARING_MODE_EXCLUSIVE);
	VkCommandBufferAllocateInfo InitCommandBufferAllocateInfo(VkCommandPool _commandPool, uint32_t _commandBufferCount, VkCommandBufferLevel _level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VkImageMemoryBarrier InitImageMemoryBarrier(VkImage _image, VkImageLayout _oldLayout, VkImageLayout _newLayout, uint32_t mipLevels = 1, bool hasStencilComponent = false, VkAccessFlagBits _srcAccessMask = VK_ACCESS_NONE, VkAccessFlagBits _dstAccessMask = VK_ACCESS_NONE, uint32_t _srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, uint32_t _dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);
	VkBufferImageCopy InitBufferImageCopy(VkDeviceSize _bufferOffset, uint32_t _bufferRowLength, uint32_t _bufferImageHeight, VkImageAspectFlags _aspectMask, VkOffset3D _imageOffset, VkExtent3D _imageExtent, uint32_t _miplevel = 0);
	VkImageBlit InitImageBlit(VkOffset3D srcOffset_luc, VkOffset3D srcOffset_rdc, VkOffset3D dstOffset_luc, VkOffset3D dstOffset_rdc, uint32_t src_miplevel = 0, uint32_t dst_miplevel = 1, VkImageAspectFlagBits src_aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, VkImageAspectFlagBits dst_aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);
	VkDescriptorSetLayoutBinding InitDescriptorSetLayoutBinding(uint32_t _binding, VkDescriptorType _descriptorType, uint32_t _descriptorCount, VkShaderStageFlagBits _stageFlags);
	VkDescriptorSetLayoutCreateInfo InitDescriptorSetLayoutCreateInfo(uint32_t _bindingCount, VkDescriptorSetLayoutBinding* bindings);
	VkDescriptorPoolCreateInfo InitDescriptorPoolCreateInfo(uint32_t _poolSizeCount, VkDescriptorPoolSize* _poolsizes, uint32_t _maxSets);
	VkDescriptorSetAllocateInfo InitDescriptorSetAllocateInfo(VkDescriptorPool _descriptorPool, uint32_t _descriptorSetCount, VkDescriptorSetLayout* _pSetLayouts);
	VkDescriptorBufferInfo InitDescriptorBufferInfo(VkBuffer _buffer, VkDeviceSize _range, VkDeviceSize _offset = 0);
	VkWriteDescriptorSet InitWriteDescriptorSet(VkDescriptorSet _dstSet, uint32_t _dstBinding, uint32_t _dstArrayElement, VkDescriptorType _descriptorType, uint32_t _descriptorCount, VkDescriptorBufferInfo* _pBufferInfo = nullptr, VkDescriptorImageInfo* _pImageInfo = nullptr, VkBufferView* _pTexelBufferView = nullptr);
	VkDescriptorImageInfo InitDescriptorImageInfo(VkImageLayout _imageLayout, VkImageView _imageView, VkSampler _sampler);
	VkAttachmentDescription InitAttachmentDescription(VkFormat format, VkImageLayout finalLayout, VkAttachmentStoreOp storeOp, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE);
}
#endif