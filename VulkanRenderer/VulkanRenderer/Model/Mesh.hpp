#pragma once
#ifndef Mesh_HPP
#define Mesh_HPP
#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <array>
#include <vector>
#include "Material.hpp"
#include "Renderer.h"
struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoords;
	static VkVertexInputBindingDescription GetBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}
	static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptons() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescription{};
		attributeDescription[0].binding = 0;
		attributeDescription[0].location = 0;
		attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[0].offset = offsetof(Vertex, position);

		attributeDescription[1].binding = 0;
		attributeDescription[1].location = 1;
		attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[1].offset = offsetof(Vertex, normal);

		attributeDescription[2].binding = 0;
		attributeDescription[2].location = 2;
		attributeDescription[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescription[2].offset = offsetof(Vertex, texCoords);
		return attributeDescription;
	}
	bool operator==(const Vertex& other) const {
		return position == other.position && normal == other.normal && texCoords == other.texCoords;
	}
};

class Mesh {
public:
	Mesh(std::vector<Vertex>& _vertices, std::vector<unsigned int>& _indices, Material _material) :vertices(_vertices), indices(_indices), material(_material) {
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		CreateBuffer(vertices.data(), bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,vertexBuffer, vertexBufferMemory, "vertexBuffer");
		bufferSize = sizeof(indices[0]) * indices.size();
		CreateBuffer(indices.data(), bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,indexBuffer, indexBufferMemory, "indexBuffer");
	}

	void Draw(VkPipelineLayout pipelineLayout, VkCommandBuffer commandBuffer);
public:
	Material material;
	void Clean() {
		Renderer* instance = Renderer::GetInstance();
		vkDestroyBuffer(instance->device, vertexBuffer, nullptr);
		vkDestroyBuffer(instance->device, indexBuffer, nullptr);
		vkFreeMemory(instance->device, vertexBufferMemory, nullptr);
		vkFreeMemory(instance->device, indexBufferMemory, nullptr);
		vertices.clear();
		indices.clear();
	}
private:
	std::vector<Vertex>			vertices;
	std::vector<unsigned int>	indices;
	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
	VkBuffer indexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

private:
	//latter, need to implement single buffer(vertex + index)
	template <typename T>
	inline void CreateBuffer(T* src, VkDeviceSize bufferSize, VkBufferUsageFlagBits usages, VkBuffer& outBuffer, VkDeviceMemory& outBufferMemory, std::string purpose = "") {
		Renderer* instance = Renderer::GetInstance();
		if (instance == nullptr) {
			printf("Fail to create %s Buffer. Please create Renderer instance or call Renderer::init()\n", purpose.c_str());
			return;
		}
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		Utils::CreateBuffer(instance->device, instance->physicalDevice, bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory, VK_SHARING_MODE_EXCLUSIVE
		);
		void* data;
		vkMapMemory(instance->device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, src, (size_t)bufferSize);
		vkUnmapMemory(instance->device, stagingBufferMemory);

		Utils::CreateBuffer(instance->device, instance->physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usages, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outBuffer, outBufferMemory);
		Utils::CopyBuffer(instance->device, instance->commandPool, instance->graphicsQueue, stagingBuffer, outBuffer, bufferSize);

		vkDestroyBuffer(instance->device, stagingBuffer, nullptr);
		vkFreeMemory(instance->device, stagingBufferMemory, nullptr);
	}
};
#endif // !Mesh_HPP
