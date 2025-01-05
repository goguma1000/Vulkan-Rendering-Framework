#pragma once
#ifndef LIGHTS_HPP
#define LIGHTS_HPP

#include <glm/glm.hpp>
struct DirectionalLight{
	glm::vec3 direction;
	float intensity = 0.5f;
	float zNear = 0.1f;
	DirectionalLight(): direction(glm::normalize(glm::vec3(1, 1, 0))), intensity(0.5f){}
	DirectionalLight(glm::vec3 dir, float _intensity = 0.5f, float _zNear = 0.1f) : direction(dir), intensity(_intensity), zNear(_zNear){}
};

#endif // !LIGHTS_HPP
