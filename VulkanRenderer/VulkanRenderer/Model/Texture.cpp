#include "Texture.hpp";
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "Model/Model.hpp"
void Texture::Show(VkCommandBuffer commandBuffer, uint32_t currentFrame, VkImageLayout imgeLayout, VkSampler sampler) {
	if (PrimitiveMesh::quad.GetMeshCount() < 1) {
		PrimitiveMesh::CreateQuad();
	}
	Renderer* instance = Renderer::GetInstance();
	VkPipelineLayout pipelineLayout = instance->GetTextureDebugPipelineLayout();
	VkPipeline pipeline = instance->GetTextureDebugPipeline();
	VkDescriptorSet descriptorSet = instance->GetTextureDebugDescriptorSet(currentFrame);
	if (sampler == VK_NULL_HANDLE) {
		sampler = instance->GetDefaultSampler();
	}
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	VkDescriptorImageInfo imageInfo = Initializer::InitDescriptorImageInfo(imgeLayout, textureImageView, sampler);
	VkWriteDescriptorSet write = Initializer::InitWriteDescriptorSet(descriptorSet, 0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, nullptr, &imageInfo);
	vkUpdateDescriptorSets(instance->device, 1, &write, 0, nullptr);
	PrimitiveMesh::RenderQuad(commandBuffer,glm::mat4(1),instance->GetTextureDebugPipelineLayout());
}
namespace Utils {
	
}