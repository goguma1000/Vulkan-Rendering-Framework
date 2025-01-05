#include "Model.hpp"
#include <cstring>
#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>
Model PrimitiveMesh::quad;

void Model::Draw(VkCommandBuffer commadbuffer,VkPipelineLayout pipelineLayout ,VkDescriptorSet texDescriptorSet, VkSampler sampler, glm::mat4 modelMat) {
	Renderer* renderer = Renderer::GetInstance();
	if (texDescriptorSet != VK_NULL_HANDLE) {
		for (int i = 0; i < texture_loaded.size(); i++) {
			VkDescriptorImageInfo imageInfo = Initializer::InitDescriptorImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture_loaded[i].textureImageView, sampler);
			VkWriteDescriptorSet write = Initializer::InitWriteDescriptorSet(texDescriptorSet, 0, i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, nullptr, &imageInfo);
			vkUpdateDescriptorSets(renderer->device, 1, &write, 0, nullptr);
		}
	}
	if (pipelineLayout == VK_NULL_HANDLE) pipelineLayout = renderer->GetPipelineLayout();
	for (int i = 0; i < meshes.size(); i++) {
		GlobalStructs::VertexShaderPushConstant pushconstant{};
		pushconstant.modelMat = GetModelMat(modelMat);
		vkCmdPushConstants(commadbuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GlobalStructs::VertexShaderPushConstant), &pushconstant);
		meshes[i].Draw(pipelineLayout,commadbuffer);
	}
}

void Model::Clean() {

	for (int i = 0; i < texture_loaded.size(); i++) {
		texture_loaded[i].Clean();
	}
	for (int i = 0; i < meshes.size(); i++) {
		meshes[i].Clean();
	}
}

void Model::LoadModel(const Renderer* renderer ,const std::string& fn) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fn, aiProcess_Triangulate);
	std::string path = Utils::getPath(fn);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::string errMsg = "ERROR::ASSIMP::";
		errMsg.append(importer.GetErrorString());
		throw std::runtime_error(errMsg.c_str());
	}
	ProcessNode(renderer, scene->mRootNode, scene, path);
}

void Model::SetPosition(float x, float y, float z) {
	position.x = x;
	position.y = y;
	position.z = z;
}

void Model::SetPosition(glm::vec3 pos) {
	position = pos;
}

glm::mat4 Model::GetModelMat(glm::mat4 modelMat) {
	return glm::translate(glm::mat4(1.0f), position) * modelMat;
}
void Model::ProcessNode(const Renderer* renderer, aiNode* node, const aiScene* scene, const std::string& path) {
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(ProcessMesh(renderer, mesh, scene, path));
	}
	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		ProcessNode(renderer, node->mChildren[i], scene, path);
	}
}

Mesh Model::ProcessMesh(const Renderer* renderer, aiMesh* mesh, const aiScene* scene, const std::string& path) {
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	Material material;
	aiString file;
	aiGetMaterialString(scene->mMaterials[mesh->mMaterialIndex], AI_MATKEY_NAME, &file);
	//process vertex
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		Vertex vertex;
		glm::vec3 vector;
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.position = vector;
		if (mesh->HasNormals()) {
			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
		}
		else {

		}
		vertex.normal = vector;
		if (mesh->mTextureCoords[0]) {
			glm::vec2 vec;
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vertex.texCoords = vec;
		}
		vertices.push_back(vertex);
	}
	//process indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}
	//process material
	if (mesh->mMaterialIndex >= 0) {
		aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
		if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &file) == AI_SUCCESS) {
			printf("Loading diffuse map : %s\n", file.C_Str());
			material.diffTexIdx = TestLoadMaterialTexture(renderer, mat, path + std::string(file.C_Str()), true);
		}
		if (mat->GetTexture(aiTextureType_SPECULAR, 0, &file) == AI_SUCCESS) {
			printf("Loading specular map : %s\n", file.C_Str());
			material.specTexIdx = TestLoadMaterialTexture(renderer, mat, path + std::string(file.C_Str()), true);
		}
		if (mat->GetTexture(aiTextureType_EMISSIVE, 0, &file) == AI_SUCCESS) {
			printf("Loading emissive map : %s\n", file.C_Str());
			material.emissionMapIdx = TestLoadMaterialTexture(renderer, mat, path + std::string(file.C_Str()), true);
		}
		if (mat->GetTexture(aiTextureType_HEIGHT, 0, &file) == AI_SUCCESS) {
			printf("Loading height map : %s\n", file.C_Str());
			material.bumpMapIdx= TestLoadMaterialTexture(renderer, mat, path + std::string(file.C_Str()), true);
		}
		if (mat->GetTexture(aiTextureType_NORMALS, 0, &file) == AI_SUCCESS) {
			printf("Loading Normal map : %s\n", file.C_Str());
			material.normalMapIdx = TestLoadMaterialTexture(renderer, mat, path + std::string(file.C_Str()), false);
		}
		if (mat->GetTexture(aiTextureType_SHININESS, 0, &file) == AI_SUCCESS) {
			printf("Loading shininess map : %s\n", file.C_Str());
			material.roughnessMapIdx = TestLoadMaterialTexture(renderer, mat, path + std::string(file.C_Str()), true);
		}
		if (mat->GetTexture(aiTextureType_OPACITY, 0, &file) == AI_SUCCESS) {
			printf("Loading opacity map : %s\n", file.C_Str());
			material.opacityMapIdx = TestLoadMaterialTexture(renderer, mat, path + std::string(file.C_Str()), true, false);
		}
		if (mat->GetTexture(aiTextureType_DISPLACEMENT, 0, &file) == AI_SUCCESS) {
			printf("Loading displacement map (as bump) : %s\n", file.C_Str());
			material.bumpMapIdx = TestLoadMaterialTexture(renderer, mat, path + std::string(file.C_Str()), true);
		}
		if (mat->GetTexture(aiTextureType_REFLECTION, 0, &file) == AI_SUCCESS) {
			printf("Loading reflection map (as spec) : %s\n", file.C_Str());
			material.specTexIdx = TestLoadMaterialTexture(renderer, mat, path + std::string(file.C_Str()), true);
		}
		if (mat->GetTexture(aiTextureType_BASE_COLOR, 0, &file) == AI_SUCCESS) {
			printf("Loading base color map (as diff) : %s\n", file.C_Str());
			material.diffTexIdx = TestLoadMaterialTexture(renderer, mat, path + std::string(file.C_Str()), true);
		}
		if (mat->GetTexture(aiTextureType_NORMAL_CAMERA, 0, &file) == AI_SUCCESS) {
			printf("Loading normal camera map (as normal): %s\n", file.C_Str());
			material.normalMapIdx = TestLoadMaterialTexture(renderer, mat, path + std::string(file.C_Str()), false);
		}
		if (mat->GetTexture(aiTextureType_EMISSION_COLOR, 0, &file) == AI_SUCCESS) {
			printf("Loading emission color map (as emissive): %s\n", file.C_Str());
			material.emissionMapIdx = TestLoadMaterialTexture(renderer, mat, path + std::string(file.C_Str()), true);
		}
		if (mat->GetTexture(aiTextureType_METALNESS, 0, &file) == AI_SUCCESS) {
			printf("Loading metalness map : %s\n", file.C_Str());
			material.metalnessMapIdx = TestLoadMaterialTexture(renderer, mat, path + std::string(file.C_Str()), false);
		}
		if (mat->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &file) == AI_SUCCESS) {
			printf("Loading amb occlusion map : %s\n", file.C_Str());
			material.ambOcclMapIdx = TestLoadMaterialTexture(renderer, mat, path + std::string(file.C_Str()), false);
		}
		if (mat->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &file) == AI_SUCCESS) {
			printf("Loading PBR Roughness map : %s\n", file.C_Str());
			material.roughnessMapIdx = TestLoadMaterialTexture(renderer, mat, path + std::string(file.C_Str()), false);
		}
	}
	return Mesh(vertices,indices,material);
}
void Model::PushMesh(Mesh& mesh) {
	meshes.push_back(mesh);
}
int Model::TestLoadMaterialTexture(const Renderer* renderer, aiMaterial* mat, const std::string& path, bool sRGB, bool genMipmap) {
	for (unsigned int i = 0; i < texture_loaded.size(); i++) {
		if (std::strcmp(texture_loaded[i].path.c_str(), path.c_str()) == 0) {
			printf("Already loaded this texture : %s\n", path.substr(path.rfind('/') + 1, path.size()).c_str());
			return i;
		}
	}
	texture_loaded.emplace_back(path);
	texture_loaded.back().Load(path, sRGB, false, genMipmap);
	return texture_loaded.size() - 1;
	
	return 0;
}


void PrimitiveMesh::createQuad() {
	std::vector<Vertex> vertices = {
		{{-1.0f, -1.0f, 0.0f}, {0.0f,0.0f,-1.0f}, {0.0f, 1.0f}},
		{{-1.0f,  1.0f, 0.0f}, {0.0f,0.0f,-1.0f}, {0.0f, 0.0f}},
		{{ 1.0f,  1.0f, 0.0f}, {0.0f,0.0f,-1.0f}, {1.0f, 0.0f}},
		{{ 1.0f, -1.0f, 0.0f}, {0.0f,0.0f,-1.0f}, {1.0f, 1.0f}}
	};
	std::vector<unsigned int>indices = {0, 1, 2, 2, 3, 0};
	Mesh temp = Mesh(vertices, indices, Material{});
	quad.PushMesh(temp);
}

Model PrimitiveMesh::CreateQuad() {
	if (quad.GetMeshCount() < 1) {
		createQuad();
	}
	return quad;
}

void PrimitiveMesh::RenderQuad(VkCommandBuffer commandBuffer, glm::mat4 modelMat, VkPipelineLayout pipelineLayout, VkDescriptorSet texDescriptorSet, VkSampler sampler) {
	if (quad.GetMeshCount() < 1) CreateQuad();
	quad.Draw(commandBuffer,pipelineLayout,texDescriptorSet,sampler);
}