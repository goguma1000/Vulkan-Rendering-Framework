#pragma once
#ifndef MODEL_HPP
#define MODEL_HPP
#include "Mesh.hpp"
#include "Texture.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/GltfMaterial.h>
#include <vector>
#include <glm/glm.hpp>
class Model {
public:
	glm::vec3 position = glm::vec3(0);
public:
	Model() { };
	Model(const Renderer* renderer, char* fn) {
		LoadModel(renderer, fn);
	}
	void Clean();
	void Draw(VkCommandBuffer commandBuffer ,VkPipelineLayout pipelineLayout = VK_NULL_HANDLE ,VkDescriptorSet texDescriptorSet = VK_NULL_HANDLE, VkSampler sampler = VK_NULL_HANDLE, glm::mat4 modelMat = glm::mat4(1));
	void LoadModel(const Renderer* renderer, const std::string& fn);
	void PushMesh(Mesh& mesh);
	int GetMeshCount() const { return meshes.size(); }
	void SetPosition(float x, float y, float z);
	void SetPosition(glm::vec3 pos);
	glm::mat4 GetModelMat(glm::mat4 modelMat = glm::mat4(1));

	VkImageView GetTextureView(int idx) { return texture_loaded[idx].textureImageView; }
private:
	std::vector<Mesh> meshes;
	std::vector<Texture> texture_loaded;
private:
	void ProcessNode(const Renderer* renderer, aiNode* node, const aiScene* scene, const std::string& path);
	Mesh ProcessMesh(const Renderer* renderer, aiMesh* mesh, const aiScene* scene, const std::string& path);
	int TestLoadMaterialTexture(const Renderer* renderer, aiMaterial * mat, const std::string& path, bool sRGB, bool genMipmap = true);
};


struct PrimitiveMesh {
	static Model quad;
	static Model CreateQuad();
	static void RenderQuad(VkCommandBuffer commandBuffer, glm::mat4 modelMat = glm::mat4(1), VkPipelineLayout pipelineLayout = VK_NULL_HANDLE, VkDescriptorSet texDescriptorSet = VK_NULL_HANDLE, VkSampler sampler = VK_NULL_HANDLE);
private:
	static void createQuad();
};

#endif // !1