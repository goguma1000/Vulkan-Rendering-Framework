#include <Tools/SamplerBuilder.hpp>

namespace SamplerBuilder {
	void SamplerBuilder::CreateSampler(VkDevice device, VkSampler& out, const VkSamplerCreateInfo& samplerInfo) {
		if (vkCreateSampler(device, &samplerInfo, nullptr, &out) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}

	VkSamplerCreateInfo SamplerBuilder::InitSamplerCreateInfo(
		float _maxLod, float _minLod, float _mipLodBias, VkSamplerMipmapMode _mipmapMode,
		VkBool32 _anisotropyEnable , float _maxAnisotropy, VkBool32 _compareEnable, VkCompareOp _compareOP,
		VkFilter _magFilter, VkFilter _minFilter,
		VkSamplerAddressMode _addressModeU, VkSamplerAddressMode _addressModeV, VkSamplerAddressMode _addressModeW,
		VkBorderColor _borderColor, VkBool32 _unnormalizeCoordinates
	) {

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = _magFilter;
		samplerInfo.minFilter = _minFilter;
		samplerInfo.addressModeU = _addressModeU;
		samplerInfo.addressModeV = _addressModeV;
		samplerInfo.addressModeW = _addressModeW;
		samplerInfo.anisotropyEnable = _anisotropyEnable;
		samplerInfo.maxAnisotropy = _maxAnisotropy;
		samplerInfo.borderColor = _borderColor;
		samplerInfo.unnormalizedCoordinates = _unnormalizeCoordinates;
		samplerInfo.compareEnable = _compareEnable;
		samplerInfo.compareOp = _compareOP;
		samplerInfo.mipmapMode = _mipmapMode;
		samplerInfo.mipLodBias = _mipLodBias;
		samplerInfo.minLod = _minLod;
		samplerInfo.maxLod = _maxLod; //device의 최대 miplevel지원값으로.

		return samplerInfo;
	}
}