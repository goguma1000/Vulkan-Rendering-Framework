#include<Tools/DescriptorBuilder.hpp>
#include<Tools/Utils.hpp>
#include<stdexcept>
namespace DescriptorBuilder {

	void DescriptorBuilder::CreateBindlessDescriptorSets(VkDevice device,VkDescriptorSetLayout& outLayout, VkDescriptorPool& outPool, std::vector<VkDescriptorSet>& outSets, const uint32_t setCount, const uint32_t descriptorCount) {
		VkDescriptorSetLayoutBinding set_binding = Initializer::InitDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorCount, VK_SHADER_STAGE_FRAGMENT_BIT);
		VkDescriptorSetLayoutCreateInfo createInfo = Initializer::InitDescriptorSetLayoutCreateInfo(1,&set_binding);
		createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;

		const VkDescriptorBindingFlagsEXT flags =
			VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT |
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
			VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
			VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;
		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT binding_flags{};
		binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		binding_flags.bindingCount = 1;
		binding_flags.pBindingFlags = &flags;

		createInfo.pNext = &binding_flags;
		if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &outLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout");
		};
		VkDescriptorPoolSize poolSize;
		poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize.descriptorCount = descriptorCount * setCount;
		VkDescriptorPoolCreateInfo poolCreateInfo = Initializer::InitDescriptorPoolCreateInfo(1, &poolSize, setCount);
		poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
		if (vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &outPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
		std::vector<VkDescriptorSetLayout> layouts(setCount, outLayout);
		VkDescriptorSetAllocateInfo allocInfo = Initializer::InitDescriptorSetAllocateInfo(outPool, static_cast<uint32_t>(layouts.size()), layouts.data());
		VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variable_info{};
		variable_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
		variable_info.descriptorSetCount = setCount;
		variable_info.pDescriptorCounts = &descriptorCount;
		allocInfo.pNext = &variable_info;
		outSets.resize(setCount);
		if (vkAllocateDescriptorSets(device, &allocInfo, outSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
	}

	void DescriptorBuilder::CreateVertexUBO_DescriptorSets(VkDevice device, VkDescriptorSetLayout& outLayout, VkDescriptorPool& outPool, std::vector<VkDescriptorSet>& outSets, const uint32_t setCount) {
		VkDescriptorSetLayoutBinding binding = Initializer::InitDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
		VkDescriptorSetLayoutCreateInfo createInfo = Initializer::InitDescriptorSetLayoutCreateInfo(1, &binding);
		if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &outLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create DescriptorSetLayout");
		}
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = 1 * MAX_FRAMES_IN_FLIGHT;
		VkDescriptorPoolCreateInfo poolInfo = Initializer::InitDescriptorPoolCreateInfo(1, &poolSize, MAX_FRAMES_IN_FLIGHT);
		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &outPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create DescriptorPool");
		}
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, outLayout);
		VkDescriptorSetAllocateInfo allocInfo = Initializer::InitDescriptorSetAllocateInfo(outPool, static_cast<uint32_t>(layouts.size()), layouts.data());
		outSets.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(device, &allocInfo, outSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create DescriptorSet");
		}
	}
}