#include "Tools/Utils.hpp"
#include "GlobalStructs.hpp"

namespace Utils
{
	std::vector<const char*> Utils::GetRequiredExtension(bool enableValidationLayer) {
		uint32_t glfwExtensionCount(0);
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		if (enableValidationLayer) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return extensions;
	}

	bool Utils::CheckValidationLayerSupport(const std::vector<const char*>& validationLayers) {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
		std::cout << "available Validation Layers: \n";
		for (const auto& availableLayer : availableLayers) {
			std::cout << '\t' << availableLayer.layerName << '\n';
		}
		for (const char* layerName : validationLayers) {
			bool layerFound(false);
			for (const auto& layerProperty : availableLayers) {
				if (std::strcmp(layerName, layerProperty.layerName) == 0) {
					layerFound = true;
					break;
				}
			}
			if (!layerFound) return false;
		}
		return true;
	}

	void Utils::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = Utils::debugCallback;
	}

	VkResult Utils::CreateDebugUtilsMessengerEXT(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger) {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void Utils::DestroyDebugUtilsMessengerEXT(VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, pAllocator);
		}
	}

	Utils::QueueFamilyIndices Utils::FindQueueFamiles(VkPhysicalDevice device, VkSurfaceKHR surface) {
		Utils::QueueFamilyIndices result;
		uint32_t queueFamilyCount(0);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {//write 2024 - 08 - 15__03:10
				result.graphicsFamily = i;
			}
			VkBool32 presentSupport(false); //write 2024 - 08 - 15__03:56
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (presentSupport) {
				result.presentFamily = i;
			}

			if (result.isComplete()) break;
			++i;
		}
		return result;
	}

	Utils::SwapChainSupportDetails Utils::QuerrySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
		Utils::SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}
		return details;
	}

	VkImageView Utils::CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageViewType viewType, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = viewType;
		viewInfo.format = format;
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}

		return imageView;
	}

	VkFormat Utils::findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}
		throw std::runtime_error("failed to find supported format!");
	}

	VkFormat Utils::findDepthFormat(VkPhysicalDevice physicalDevice) {
		return findSupportedFormat(physicalDevice,
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	uint32_t Utils::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
		throw std::runtime_error("failed to find suitable memory type!");
	}

	void Utils::CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkSharingMode sharingMode, VkDeviceSize memoffset) {
		VkBufferCreateInfo bufferInfo = Initializer::InitBufferCreateInfo(size, usage, sharingMode);
		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = Initializer::InitMemoryAllocateInfo(memRequirements.size, findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties));
		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate vertex buffer memory!");
		}
		vkBindBufferMemory(device, buffer, bufferMemory, memoffset);
	}
	void Utils::CopyBuffer(VkDevice device, VkCommandPool commandPool,VkQueue submitQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize _size) {
		VkCommandBuffer commandBuffer = BeginSingleTimeCommand(device, commandPool);

		VkBufferCopy copyRegion{};
		copyRegion.size = _size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
		EndSingleTimeCommand(device, commandPool, submitQueue, commandBuffer);

	}
	void Utils::CreateVertexUniforBuffer(VkDevice device,VkPhysicalDevice physicalDevice ,std::vector<VkBuffer>& buffer, std::vector<VkDeviceMemory>& bufferMemory, std::vector<void*>& mappedMemory, uint32_t bufferCount) {
		buffer.resize(bufferCount);
		bufferMemory.resize(bufferCount);
		mappedMemory.resize(bufferCount);
		for (int i = 0; i < bufferCount; i++) {
			Utils::CreateBuffer(device, physicalDevice, sizeof(GlobalStructs::VertexShaderUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,buffer[i],bufferMemory[i]);
			vkMapMemory(device, bufferMemory[i], 0, sizeof(GlobalStructs::VertexShaderUBO), 0, &mappedMemory[i]);
		}
	}

	void Utils::CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, VkImage& image, VkDeviceMemory& imageMemory, VkMemoryPropertyFlags properties, VkImageCreateInfo& imageInfo, VkDeviceSize memoryOffset) {
		if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image!");
		}
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo = Initializer::InitMemoryAllocateInfo(memRequirements.size, findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties));
		if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate Image memory!");
		}
		vkBindImageMemory(device, image, imageMemory, memoryOffset);
	}
	VkCommandBuffer Utils::BeginSingleTimeCommand(VkDevice device, VkCommandPool commandPool) {
		VkCommandBufferAllocateInfo allocInfo = Initializer::InitCommandBufferAllocateInfo(commandPool, 1);
		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = Initializer::InitCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		return commandBuffer;
	}

	void Utils::EndSingleTimeCommand(VkDevice device, VkCommandPool commandPool, VkQueue submitQueue, VkCommandBuffer commandBuffer) {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = Initializer::InitSubmitInfo(0, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &commandBuffer, 0, VK_NULL_HANDLE);
		vkQueueSubmit(submitQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(submitQueue);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}
	void Utils::transitionImageLayout(VkDevice device, VkCommandPool commandPool,VkQueue submitQueue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
		VkCommandBuffer commandBuffer = BeginSingleTimeCommand(device,commandPool);
		bool hasStencilComponent = format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT ? true : false;
		VkImageMemoryBarrier barrier = Initializer::InitImageMemoryBarrier(image,oldLayout,newLayout,mipLevels,hasStencilComponent);
		VkPipelineStageFlagBits sourceStage;
		VkPipelineStageFlagBits destinationStage;
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else {
			throw std::runtime_error("unsupported layout transition!");
		}
		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
		EndSingleTimeCommand(device, commandPool, submitQueue, commandBuffer);
	}

	std::string Utils::getPath(const std::string& filename) {
		size_t slashPos = filename.find_last_of('/');
		if (slashPos == std::string::npos) return "";
		if (slashPos == filename.length() - 1) return filename;
		return filename.substr(0, slashPos + 1);
	}
}

namespace Initializer {
	VkSemaphoreCreateInfo Initializer::InitSemaphoreCreateInfo(void* next, VkSemaphoreCreateFlags flag) {
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreInfo.pNext = NULL;
		semaphoreInfo.flags = flag;
		return semaphoreInfo;
	}

	VkFenceCreateInfo Initializer::InitFenceCreateInfo(VkFenceCreateFlagBits flag, void* next) {
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = flag;
		fenceInfo.pNext = next;
		return fenceInfo;
	}

	VkSubmitInfo Initializer::InitSubmitInfo(uint32_t _waitSemaphoreCount, VkSemaphore* _pWaitSemaphores, VkPipelineStageFlags* _pWaitDstStageMask,
		uint32_t _commandBufferCount, VkCommandBuffer* _pCommandBuffers, uint32_t _signalSemaphoreCount, VkSemaphore* _pSignalSemaphores
	) {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = _waitSemaphoreCount;
		submitInfo.pWaitSemaphores = _pWaitSemaphores;
		submitInfo.pWaitDstStageMask = _pWaitDstStageMask;
		submitInfo.commandBufferCount = _commandBufferCount;
		submitInfo.pCommandBuffers = _pCommandBuffers;
		submitInfo.signalSemaphoreCount = _signalSemaphoreCount;
		submitInfo.pSignalSemaphores = _pSignalSemaphores;
		return submitInfo;
	}

	VkPresentInfoKHR Initializer::InitPresentInfo(uint32_t _waitSemaphoreCount, VkSemaphore* _pWaitSemaphores,
		uint32_t _swapChainCount, VkSwapchainKHR* _pSwapChains, uint32_t* _pImageIndices, VkResult* _pResult) {
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = _waitSemaphoreCount;
		presentInfo.pWaitSemaphores = _pWaitSemaphores;
		presentInfo.swapchainCount = _swapChainCount;
		presentInfo.pSwapchains = _pSwapChains;
		presentInfo.pImageIndices = _pImageIndices;
		presentInfo.pResults = _pResult;
		return presentInfo;
	}

	VkCommandBufferBeginInfo Initializer::InitCommandBufferBeginInfo(VkCommandBufferUsageFlags _flags, VkCommandBufferInheritanceInfo* _pInheritanceInfo, void* _pNext) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = _flags;
		beginInfo.pInheritanceInfo = _pInheritanceInfo;
		beginInfo.pNext = _pNext;
		return beginInfo;
	}

	VkRenderPassBeginInfo Initializer::InitRenderPassBeginInfo(VkRenderPass _renderPass, VkFramebuffer _framebuffer, VkOffset2D _offset, VkExtent2D swapChainExtent, uint32_t _clearValueCount, VkClearValue* _pClearValues) {
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = _renderPass;
		renderPassInfo.framebuffer = _framebuffer;
		renderPassInfo.renderArea.offset = _offset;
		renderPassInfo.renderArea.extent = swapChainExtent;
		renderPassInfo.clearValueCount = _clearValueCount;
		renderPassInfo.pClearValues = _pClearValues;
		return renderPassInfo;
	}
	VkViewport Initializer::InitViewport(float _x, float _y, float _width, float _height, float _minDepth, float _maxDepth) {
		VkViewport viewport{};
		viewport.x = _x;
		viewport.y = _y;
		viewport.width = _width;
		viewport.height = _height;
		viewport.minDepth = _minDepth;
		viewport.maxDepth = _maxDepth;
		return viewport;
	}

	VkRect2D Initializer::InitScissor(VkOffset2D _offset, VkExtent2D _extent) {
		VkRect2D scissor{};
		scissor.offset = _offset;
		scissor.extent = _extent;
		return scissor;
	}
	VkBufferCreateInfo Initializer::InitBufferCreateInfo(VkDeviceSize _size, VkBufferUsageFlags _usage, VkSharingMode _sharingMode) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = _size;
		bufferInfo.usage = _usage;
		bufferInfo.sharingMode = _sharingMode;
		return bufferInfo;
	}
	VkMemoryAllocateInfo Initializer::InitMemoryAllocateInfo(VkDeviceSize _allocationSize, uint32_t _memoryTypeIndex) {
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = _allocationSize;
		allocInfo.memoryTypeIndex = _memoryTypeIndex;
		return allocInfo;
	}
	VkImageCreateInfo Initializer::InitImageCreateInfo(VkImageType _imageType, uint32_t _width, uint32_t _height, uint32_t _depth, uint32_t _miplevels,
		VkFormat _format, VkImageTiling _tiling, VkImageUsageFlags _usage,
		VkSampleCountFlagBits numSamples, VkImageLayout _initialLayout, VkSharingMode _sharingMode) {
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = _imageType;
		imageInfo.extent.width = _width;
		imageInfo.extent.height = _height;
		imageInfo.extent.depth = _depth;
		imageInfo.mipLevels = _miplevels;
		imageInfo.arrayLayers = 1;
		imageInfo.format = _format;
		imageInfo.tiling = _tiling;
		imageInfo.initialLayout = _initialLayout;
		imageInfo.usage = _usage;
		imageInfo.sharingMode = _sharingMode;
		imageInfo.samples = numSamples;
		imageInfo.flags = 0;
		return imageInfo;
	}

	VkCommandBufferAllocateInfo Initializer::InitCommandBufferAllocateInfo(VkCommandPool _commandPool, uint32_t _commandBufferCount, VkCommandBufferLevel _level) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = _level;
		allocInfo.commandPool = _commandPool;
		allocInfo.commandBufferCount = 1;
		return allocInfo;
	}

	VkImageMemoryBarrier Initializer::InitImageMemoryBarrier(VkImage _image, VkImageLayout _oldLayout, VkImageLayout _newLayout, uint32_t mipLevels, bool hasStencilComponent, VkAccessFlagBits _srcAccessMask, VkAccessFlagBits _dstAccessMask, uint32_t _srcQueueFamilyIndex, uint32_t _dstQueueFamilyIndex) {
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = _oldLayout;
		barrier.newLayout = _newLayout;
		barrier.srcAccessMask = _srcAccessMask;
		barrier.dstAccessMask = _dstAccessMask;
		barrier.srcQueueFamilyIndex = _srcQueueFamilyIndex;
		barrier.dstQueueFamilyIndex = _dstQueueFamilyIndex;
		barrier.image = _image;
		if (_newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (hasStencilComponent) {
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
	
		return barrier;
	}

	VkBufferImageCopy Initializer::InitBufferImageCopy(VkDeviceSize _bufferOffset, uint32_t _bufferRowLength, uint32_t _bufferImageHeight, VkImageAspectFlags _aspectMask,VkOffset3D _imageOffset, VkExtent3D _imageExtent, uint32_t _miplevel) {
		VkBufferImageCopy region{};
		region.bufferOffset = _bufferOffset;
		region.bufferRowLength = _bufferRowLength;
		region.bufferImageHeight = _bufferImageHeight;
		region.imageSubresource.aspectMask = _aspectMask;
		region.imageSubresource.mipLevel = _miplevel;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = _imageOffset;
		region.imageExtent = _imageExtent;
		return region;
	}

	VkImageBlit Initializer::InitImageBlit(VkOffset3D srcOffset_luc, VkOffset3D srcOffset_rdc, VkOffset3D dstOffset_luc, VkOffset3D dstOffset_rdc, uint32_t src_miplevel, uint32_t dst_miplevel, VkImageAspectFlagBits src_aspectMask, VkImageAspectFlagBits dst_aspectMask) {
		VkImageBlit blit{};
		blit.srcOffsets[0] = srcOffset_luc;
		blit.srcOffsets[1] = srcOffset_rdc;
		blit.srcSubresource.aspectMask = src_aspectMask;
		blit.srcSubresource.mipLevel = src_miplevel;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = dstOffset_luc;
		blit.dstOffsets[1] = dstOffset_rdc;
		blit.dstSubresource.aspectMask = dst_aspectMask;
		blit.dstSubresource.mipLevel = dst_miplevel;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;
		return blit;
	}

	VkDescriptorSetLayoutBinding Initializer::InitDescriptorSetLayoutBinding(uint32_t _binding, VkDescriptorType _descriptorType, uint32_t _descriptorCount, VkShaderStageFlagBits _stageFlags) {
		VkDescriptorSetLayoutBinding descriptorLayoutBinding{};
		descriptorLayoutBinding.binding = _binding;
		descriptorLayoutBinding.descriptorType = _descriptorType;
		descriptorLayoutBinding.descriptorCount = _descriptorCount;
		descriptorLayoutBinding.stageFlags = _stageFlags;
		return descriptorLayoutBinding;
	}

	VkDescriptorSetLayoutCreateInfo Initializer::InitDescriptorSetLayoutCreateInfo(uint32_t _bindingCount, VkDescriptorSetLayoutBinding* bindings) {
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = _bindingCount;
		layoutInfo.pBindings = bindings;
		return layoutInfo;
	}

	VkDescriptorPoolCreateInfo Initializer::InitDescriptorPoolCreateInfo(uint32_t _poolSizeCount, VkDescriptorPoolSize* _poolsizes, uint32_t _maxSets) {
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = _poolSizeCount;
		poolInfo.pPoolSizes = _poolsizes;
		poolInfo.maxSets = _maxSets;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		return poolInfo;
	}
	VkDescriptorSetAllocateInfo Initializer::InitDescriptorSetAllocateInfo(VkDescriptorPool _descriptorPool, uint32_t _descriptorSetCount, VkDescriptorSetLayout* _pSetLayouts) {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = _descriptorPool;
		allocInfo.descriptorSetCount = _descriptorSetCount;
		allocInfo.pSetLayouts = _pSetLayouts;
		return allocInfo;
	}

	VkDescriptorBufferInfo Initializer::InitDescriptorBufferInfo(VkBuffer _buffer,VkDeviceSize _range, VkDeviceSize _offset) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = _buffer;
		bufferInfo.range = _range;
		bufferInfo.offset = _offset;
		return bufferInfo;
	}

	VkWriteDescriptorSet Initializer::InitWriteDescriptorSet(VkDescriptorSet _dstSet,uint32_t _dstBinding, uint32_t _dstArrayElement, VkDescriptorType _descriptorType, uint32_t _descriptorCount, VkDescriptorBufferInfo* _pBufferInfo, VkDescriptorImageInfo* _pImageInfo, VkBufferView* _pTexelBufferView) {
		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = _dstSet;
		write.dstBinding = _dstBinding;
		write.dstArrayElement = _dstArrayElement;
		write.descriptorType = _descriptorType;
		write.descriptorCount = _descriptorCount;
		write.pBufferInfo = _pBufferInfo;
		write.pImageInfo = _pImageInfo;
		write.pTexelBufferView = _pTexelBufferView;
		return write;
	}

	VkDescriptorImageInfo Initializer::InitDescriptorImageInfo(VkImageLayout _imageLayout, VkImageView _imageView, VkSampler _sampler) {
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = _imageLayout;
		imageInfo.imageView = _imageView;
		imageInfo.sampler = _sampler;
		return imageInfo;
	}
	VkAttachmentDescription InitAttachmentDescription(VkFormat format, VkImageLayout finalLayout, VkAttachmentStoreOp storeOp, VkSampleCountFlagBits samples, VkImageLayout initialLayout, VkAttachmentLoadOp loadOp, VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp) {
		VkAttachmentDescription attachment{};
		attachment.format = format;
		attachment.samples = samples;
		attachment.loadOp = loadOp;
		attachment.storeOp = storeOp;
		attachment.stencilLoadOp = stencilLoadOp;
		attachment.stencilStoreOp = stencilStoreOp;
		attachment.initialLayout = initialLayout;
		attachment.finalLayout = finalLayout;
		return attachment;
	}

}