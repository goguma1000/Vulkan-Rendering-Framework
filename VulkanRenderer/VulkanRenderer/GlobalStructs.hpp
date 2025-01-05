#pragma once
#ifndef GLOBAL_STRUCTS_HPP
#define GLOABL_STRUCTS_HPP
#include <glm/glm.hpp>
#include "Lights.hpp"
#include <Model/Material.hpp>
namespace GlobalStructs {
	struct VertexShaderUBO {
		glm::mat4 lightSpaceMat = glm::mat4(1);
		glm::mat4 view = glm::mat4(1);
		glm::mat4 proj = glm::mat4(1);
	};

	struct FragmentShaderUBO
	{
		DirectionalLight dirLight;
		glm::vec3 cameraPos;
	};

	struct VertexShaderPushConstant {
		glm::mat4 modelMat = glm::mat4(1);
	};

	struct TextureIndexPushConstant {
		int diffTexIdx = -1;
		int specTexIdx = -1;
		int bumpMapIdx = -1;
		int normalMapIdx = -1;
		int emissionMapIdx = -1;
		int opacityMapIdx = -1;
		int roughnessMapIdx = -1;
		int metalnessMapIdx = -1;
		int ambOcclMapIdx = -1;

		TextureIndexPushConstant(const Material& mat) :
			diffTexIdx(mat.diffTexIdx),
			specTexIdx(mat.specTexIdx),
			bumpMapIdx(mat.bumpMapIdx),
			normalMapIdx(mat.normalMapIdx),
			emissionMapIdx(mat.normalMapIdx),
			opacityMapIdx(mat.opacityMapIdx),
			roughnessMapIdx(mat.roughnessMapIdx),
			metalnessMapIdx(mat.metalnessMapIdx),
			ambOcclMapIdx(mat.ambOcclMapIdx) { }
	};
}
#endif // !GLOBAL_STRUCTS_HPP
