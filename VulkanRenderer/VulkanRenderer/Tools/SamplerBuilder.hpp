#pragma once

#ifndef SAMPLERBUILDER_HPP
#define SAMPLERBUILDER_HPP
#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include <stdexcept>
namespace SamplerBuilder {
	void CreateSampler(VkDevice device, VkSampler& out, const VkSamplerCreateInfo& samplerInfo);
	
	VkSamplerCreateInfo InitSamplerCreateInfo(
		float _maxLod = 1.0f, float _minLod = 0.0f, float _mipLodBias = 0.0f, VkSamplerMipmapMode _mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VkBool32 _anisotropyEnable = VK_FALSE, float _maxAnisotropy = 1.0f, VkBool32 _compareEnable = VK_FALSE, VkCompareOp _compareOP = VK_COMPARE_OP_ALWAYS,
		VkFilter _magFilter = VK_FILTER_LINEAR, VkFilter _minFilter = VK_FILTER_LINEAR,
		VkSamplerAddressMode _addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT, VkSamplerAddressMode _addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT, VkSamplerAddressMode _addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VkBorderColor _borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK, VkBool32 _unnormalizeCoordinates = VK_FALSE
	);
}
#endif // !SAMPLERBUILDER_HPP
