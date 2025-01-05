#include "Mesh.hpp"

void Mesh::Draw(VkPipelineLayout pipelineLayout, VkCommandBuffer commandBuffer) {
	GlobalStructs::TextureIndexPushConstant push_constant(material);
	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(GlobalStructs::VertexShaderPushConstant), sizeof(GlobalStructs::TextureIndexPushConstant), &push_constant);

	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(commandBuffer,static_cast<uint32_t>(indices.size()),1,0,0,0);
}