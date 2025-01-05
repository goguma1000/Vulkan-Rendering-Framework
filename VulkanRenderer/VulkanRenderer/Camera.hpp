#pragma once
#ifndef CAMERA_HPP
#define CAMERA_HPP
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
using namespace glm;
enum class CAMERA_MOVERMENT {
	FORWARD,
	BACK,
	LEFT,
	RIGHT,
	UP,
	DOWN,
	NONE
};

struct Camera
{
private:
	const glm::vec3 worldUp	= glm::vec3(0.0f, 1.0f, 0.0f);

	float phi = 0.0f;
	float theta = -90.0f; // To point z-vector to screen
	float lastX = 0.0f;
	float lastY = 0.0f;

	void UpdateCameraCoord(float theta, float phi) {
		vec3 forward;
		forward.x = glm::cos(theta) * glm::cos(phi);
		forward.y = glm::sin(phi);
		forward.z = glm::sin(theta) * glm::cos(phi);
		glm::normalize(forward);
		Front = forward;

		Right = normalize(glm::cross(Front, worldUp));
		Up = normalize(glm::cross(Right, Front));
	}

public:
	//camera properties
	//Y Up System
	glm::vec3 Front = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 position = glm::vec3(0, 0, 0);

	float moveSpeed = 1.0f;
	float mouseSensitivity = 0.2f;
	float zNear = 0.1f;
	float zFar = 100.0f;
	float fov = glm::radians(60.0f);

	bool firstMove = true;

	glm::mat4 GetViewMat() {
		return glm::lookAt(position, position + Front, Up);
	}
	glm::mat4 GetProjMat(float width, float height) {
		return glm::perspective(fov, width / height, zNear, zFar);
	}
	void ProcessKeyInput(CAMERA_MOVERMENT moveType, float deltaTime) {
		float velocity = moveSpeed * deltaTime;
		switch (moveType)
		{
		case CAMERA_MOVERMENT::FORWARD : {
			position += Front * velocity;
			break;
		}
		case CAMERA_MOVERMENT::BACK:{
			position -= Front * velocity;
			break;
		}
		case CAMERA_MOVERMENT::LEFT:{
			position -= Right * velocity;
			break;					   
		}
		case CAMERA_MOVERMENT::RIGHT:{
			position += Right * velocity;
			break;					   
		}
		case CAMERA_MOVERMENT::UP:{
			position += worldUp * velocity;
			break;					   
		}
		case CAMERA_MOVERMENT::DOWN:{
			position -= worldUp * velocity;
			break;					   
		}
		default:
			break;
		}
	}
	void ProcessMouseMove(float xPos, float yPos) {
		if (firstMove) {
			lastX = xPos;
			lastY = yPos;
			firstMove = false;
		}
		float xoffset = xPos - lastX;
		float yoffset = yPos - lastY;

		lastX = xPos;
		lastY = yPos;
		
		xoffset *= mouseSensitivity;
		yoffset *= mouseSensitivity;

		theta += xoffset;
		phi += yoffset;
		
		glm::clamp(phi, -89.f, 89.f);
		UpdateCameraCoord(glm::radians(theta), glm::radians(-phi));
	}

};
#endif // !CAMERA_HPP
