#pragma once
#ifndef MATERIAL_HPP
#define MATERIAL_HPP
struct  Material{
	int diffTexIdx = -1;
	int specTexIdx = -1;
	int bumpMapIdx = -1;
	int normalMapIdx = -1;
	int emissionMapIdx = -1;
	int opacityMapIdx = -1;
	int roughnessMapIdx = -1;
	int metalnessMapIdx = -1;
	int ambOcclMapIdx = -1;
};
#endif // !MATERIAL_HPP
