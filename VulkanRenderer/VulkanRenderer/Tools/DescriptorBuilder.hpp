#pragma once
#ifndef DESCRIPTOR_BUILDER_HPP
#define DESCRIPTOR_BUILDER_HPP
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include<vector>
extern const int MAX_FRAMES_IN_FLIGHT;
namespace DescriptorBuilder {
	void CreateBindlessDescriptorSets(VkDevice device, VkDescriptorSetLayout& outLayout, VkDescriptorPool& outPool, std::vector<VkDescriptorSet>& outSets, const uint32_t setCount = MAX_FRAMES_IN_FLIGHT, const uint32_t descriptorCount = 500000);
	void CreateVertexUBO_DescriptorSets(VkDevice device, VkDescriptorSetLayout& outLayout, VkDescriptorPool& outPool, std::vector<VkDescriptorSet>& outSets, const uint32_t setCount = MAX_FRAMES_IN_FLIGHT);
}
#endif // !DESCRIPTOR_BUILDER_HPP
