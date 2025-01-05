#pragma once
#ifndef Texture_HPP
#define Texture_HPP
#include<iostream>
#include<stdexcept>
#include<string>
#include<stb_image.h>
#include "Tools/Utils.hpp"
#include "Renderer.h"

using namespace std;
struct PrimitiveMesh;

struct Texture{
	VkExtent2D textureSize = { 0,0 };
	uint32_t mipLevels = 1;
	VkImage textureImage = VK_NULL_HANDLE;
	VkImageView textureImageView = VK_NULL_HANDLE;
	VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
	string path = "";
public:
	Texture(const string& _path) :path(_path) {};
	Texture() {

	}

	void Clean() {
		Renderer* instance = Renderer::GetInstance();
		vkDestroyImageView(instance->device, textureImageView, nullptr);
		vkDestroyImage(instance->device, textureImage, nullptr);
		vkFreeMemory(instance->device, textureImageMemory, nullptr);
	}
	void Create(float width, float height, VkFormat format, VkImageUsageFlags usages, VkImageAspectFlagBits flagBits, VkImageLayout nextLayout) {
		textureSize.width = width;
		textureSize.height = height;
		if (textureImage != VK_NULL_HANDLE) {
			Clean();
		}
		Renderer* instance = Renderer::GetInstance();
		if (instance == nullptr) {
			std::cout << "renderer instance is nullptr! please create renderer instance  calling GetInstance(GlfwWindow, rendererCustomFuncs)!";
			return;
		}
		VkImageCreateInfo imageInfo = Initializer::InitImageCreateInfo(VK_IMAGE_TYPE_2D, width, height, 1, 1, format, VK_IMAGE_TILING_OPTIMAL, usages);
		Utils::CreateImage(instance->device, instance->physicalDevice, textureImage, textureImageMemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, imageInfo);
		textureImageView = Utils::CreateImageView(instance->device, textureImage, format, VK_IMAGE_VIEW_TYPE_2D, flagBits);
		Utils::transitionImageLayout(instance->device, instance->commandPool, instance->graphicsQueue, textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED, nextLayout, 1);
	}
	void Load(const string& fn, bool sRGB = false, bool isHdr = false, bool genMipmap = true, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL) {
		Renderer* renderer = Renderer::GetInstance();
		if (renderer == nullptr) {
			std::cout << "renderer instance is nullptr! please create renderer instance  calling GetInstance(GlfwWindow, rendererCustomFuncs)!";
			return;
		}
		void* buf = nullptr;
		int width = 0, height = 0, nChannels = 0;
		//4채널이미지가 아니면 4채널로 만들어 줘야함 1채널과 3채널은 STBI_rgb_alpha를 쓰면 되는데 2채널은 따로 추가해줘야함
		stbi_set_flip_vertically_on_load(true);
		if (isHdr) {
			buf = (float*)stbi_loadf(fn.c_str(), &width, &height, &nChannels, STBI_rgb_alpha);
		}
		else {
			buf = (unsigned char*)stbi_load(fn.c_str(), &width, &height, &nChannels, STBI_rgb_alpha);
		}

		if (buf) {
			cout << fn << " image load success! width : " << width << " height : " << height << " channels : " << nChannels << std::endl;
		}
		else {
			throw std::runtime_error("failed to load texture image!");
		}
		VkFormat format = GetTextureFormat(sRGB, isHdr, nChannels);
		mipLevels = genMipmap ? static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1 : 1;
		
		VkDeviceSize imageSize = width * height * 4;
		//staging buffer
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		Utils::CreateBuffer(renderer->device, renderer->physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer, stagingBufferMemory
		);
		void* data;
		vkMapMemory(renderer->device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, buf, static_cast<size_t>(imageSize));
		vkUnmapMemory(renderer->device, stagingBufferMemory);
		stbi_image_free(buf);
		VkImageCreateInfo imageInfo = Initializer::InitImageCreateInfo(VK_IMAGE_TYPE_2D,static_cast<uint32_t>(width), static_cast<uint32_t>(height),1,mipLevels,format,tiling,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT);  //to generate mipmap add VK_IMAGE_USAGE_TRANSFER_SRC_BUT to usage flags
		VkImageFormatProperties proper{};
		VkResult result = vkGetPhysicalDeviceImageFormatProperties(renderer->physicalDevice, VK_FORMAT_R8G8_SRGB, VK_IMAGE_TYPE_2D, tiling, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0, &proper);
		Utils::CreateImage(renderer->device, renderer->physicalDevice, textureImage, textureImageMemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, imageInfo);
		//to generate mipmap, change VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL.
		Utils::transitionImageLayout(renderer->device, renderer->commandPool, renderer->graphicsQueue, textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
		copyBufferToImage(renderer->device, renderer->commandPool, renderer->graphicsQueue, stagingBuffer, textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
		generateMipmaps(renderer->device, renderer->commandPool, renderer->graphicsQueue, renderer->physicalDevice, textureImage, format, width, height, mipLevels);
		vkDestroyBuffer(renderer->device, stagingBuffer, nullptr);
		vkFreeMemory(renderer->device, stagingBufferMemory, nullptr);

		//create texture image view
		textureImageView = Utils::CreateImageView(renderer->device, textureImage, format, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
	}
	void Show(VkCommandBuffer commandBuffer, uint32_t currentFrame, VkImageLayout imgeLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VkSampler sampler = VK_NULL_HANDLE);
private:
inline void copyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue submitQueue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth = 1) {
		VkCommandBuffer commandbuffer = Utils::BeginSingleTimeCommand(device, commandPool);
		VkBufferImageCopy region = Initializer::InitBufferImageCopy(0, 0, 0, VK_IMAGE_ASPECT_COLOR_BIT, { 0,0,0 }, { width,height,depth});
		vkCmdCopyBufferToImage(
			commandbuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);
		Utils::EndSingleTimeCommand(device, commandPool, submitQueue, commandbuffer);
	}

inline void generateMipmaps(VkDevice device, VkCommandPool commandPool, VkQueue submitQueue, VkPhysicalDevice physicalDevice ,VkImage image, VkFormat imageFormat,int32_t width, int32_t height, uint32_t mipLevels) {
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("texture image format dose not support linear bliting!");
		//other way is resizing the image using stb_image_resize.
	}
	VkCommandBuffer commandBuffer = Utils::BeginSingleTimeCommand(device, commandPool);

	VkImageMemoryBarrier barrier = Initializer::InitImageMemoryBarrier(image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,1);
	int32_t mipWidth = width;
	int32_t mipHeight = height;

	for (uint32_t i = 1; i < mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		vkCmdPipelineBarrier(
			commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
		VkImageBlit blit = Initializer::InitImageBlit({ 0,0,0 }, { mipWidth, mipHeight,1 }, { 0,0,0 }, {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1 , 1},i - 1, i);
		vkCmdBlitImage(commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR
		);
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}
	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
	Utils::EndSingleTimeCommand(device, commandPool, submitQueue, commandBuffer);
}

inline VkFormat GetTextureFormat(bool sRGB, bool isHdr, int nChannels) {
	if (isHdr) {
		switch (nChannels)
		{
		case 1:		return VK_FORMAT_R32_SFLOAT;
		case 2:		return VK_FORMAT_R32G32_SFLOAT;
		case 4:		return VK_FORMAT_R32G32B32A32_SFLOAT;
		//default:	return VK_FORMAT_R32G32B32_SFLOAT;
		default:	return VK_FORMAT_R32G32B32A32_SFLOAT;
			break;
		}
	}
	else {
		switch (nChannels)
		{
		case 1:		return sRGB?VK_FORMAT_R8_SRGB : VK_FORMAT_R8_UNORM;
		case 2:		return sRGB?VK_FORMAT_R8G8_SRGB : VK_FORMAT_R8G8_UNORM;
		case 4:		return sRGB?VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
		//default:	return sRGB?VK_FORMAT_R8G8B8_SRGB : VK_FORMAT_R8G8B8_UNORM;
		default:	return sRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
			break;
		}
	}
}

};

namespace Utils {

}
#endif // !Texture_HPP

