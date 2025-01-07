#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include <iostream>
#include <array>
#include "Renderer.h"
#include "Camera.hpp"
#include "Tools/Utils.hpp"
#include "Tools/FrameBuffer.hpp"
#include "Tools/PipelineBuilder.hpp"
#include "Tools/SamplerBuilder.hpp"
#include "Tools/DescriptorBuilder.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "Model/Model.hpp"

void CreateShadowMap(int, VkCommandBuffer);

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
glm::vec3 pos;
GLFWwindow* window;
Renderer* renderer;
Camera mainCamera;
DirectionalLight sun;
Model model;
Model plane;
GlobalStructs::VertexShaderUBO vert_ubo{};
GlobalStructs::FragmentShaderUBO frag_ubo{};

FrameBuffer shadowFramebuffer;
Texture shadowMap;
VkSampler shadowSampler;
std::vector<VkBuffer> ShadowVertexUnifomrBuffer;
std::vector<VkDeviceMemory> ShadowVertexUniformBuffersMemory;
std::vector<void*> ShadowVertexUniformBuffersMapped;
VkRenderPass shadowMapRenderPass;
VkPipelineLayout shadowMapPipeLayout = VK_NULL_HANDLE;
VkPipeline shadowMapPipeline = VK_NULL_HANDLE;
VkDescriptorSetLayout shadowDescriptorSetLayout;
std::vector<VkDescriptorSet> shadowDescriptorSets;
VkDescriptorPool shadowDescriptorPool;


void Clean() {
	model.Clean();
	plane.Clean();
	shadowMap.Clean();
	shadowFramebuffer.Clean();
	vkDestroySampler(renderer->device, shadowSampler, nullptr);
	for (int i = 0; i < shadowDescriptorSets.size(); i++) {
		vkDestroyBuffer(renderer->device, ShadowVertexUnifomrBuffer[i], nullptr);
		vkFreeMemory(renderer->device, ShadowVertexUniformBuffersMemory[i], nullptr);
	}
	
	vkDestroyDescriptorPool(renderer->device, shadowDescriptorPool,nullptr);
	vkDestroyDescriptorSetLayout(renderer->device, shadowDescriptorSetLayout, nullptr);
	vkDestroyPipeline(renderer->device, shadowMapPipeline, nullptr);
	vkDestroyPipelineLayout(renderer->device, shadowMapPipeLayout, nullptr);
	vkDestroyRenderPass(renderer->device, shadowMapRenderPass, nullptr);
}
#pragma region Renderer custom function

bool isDeviceSuitable(VkPhysicalDevice device) {
	return true;
}

bool CheckSwapSurfaceSupport(const VkSurfaceFormatKHR& availableFormat) {
	if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB && availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
		return true;
	else return false;
}
bool CheckSwapPresentMode(const VkPresentModeKHR& availablePresentMode) {
	if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) return true;
	else return false;
}
void SetPhysicalDeviceFeatures(VkPhysicalDeviceFeatures& deviceFeatures) {
	
}

void drawFunc(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, uint32_t currentFrame) {
	//ShadowMap
	CreateShadowMap(currentFrame, commandBuffer);

	Renderer* renderer = Renderer::GetInstance();
	if (renderer == nullptr) return;

	VkExtent2D swapChainExtent = renderer->GetSwapChainExtent();
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.3f, 0.3f, 0.3f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };
	VkRenderPassBeginInfo renderPassInfo =
		Initializer::InitRenderPassBeginInfo(renderer->GetRenderPass(), framebuffer, { 0,0 }, swapChainExtent, static_cast<uint32_t>(clearValues.size()), clearValues.data());
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->GetPipeline());

	VkViewport viewport = Initializer::InitViewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f);
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor = Initializer::InitScissor({ 0,0 }, swapChainExtent);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkDescriptorSet descriptorSet = renderer->GetDescriptorSet(currentFrame);

	//update vertex ubo
	VkExtent2D swapchainExtent = renderer->GetSwapChainExtent();

	vert_ubo.view = mainCamera.GetViewMat();
	vert_ubo.proj = mainCamera.GetProjMat(swapChainExtent.width, swapchainExtent.height);
	vert_ubo.proj[1][1] *= -1;
	renderer->UpdateVertexUniformBuffer(currentFrame, vert_ubo);

	//update frament ubo
	frag_ubo.cameraPos = mainCamera.position;
	renderer->UpdateFragUniformBuffer(currentFrame, frag_ubo);

	//bind Global Texture
	VkDescriptorImageInfo shadowMapInfo = Initializer::InitDescriptorImageInfo(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, shadowMap.textureImageView, shadowSampler);
	VkWriteDescriptorSet write = Initializer::InitWriteDescriptorSet(descriptorSet, 2, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, nullptr, &shadowMapInfo);
	vkUpdateDescriptorSets(renderer->device, 1, &write, 0, nullptr);
	
	//Write here

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->GetPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);
	//bind texture
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->GetPipelineLayout(), 1, 1, &renderer->texDescriptorSets[currentFrame], 0, nullptr);
	model.SetPosition(pos);
	model.Draw(commandBuffer, renderer->GetPipelineLayout(), renderer->texDescriptorSets[currentFrame], renderer->GetDefaultSampler());
	model.SetPosition(pos + glm::vec3(0.2f, 0.1f, 0.2f));
	model.Draw(commandBuffer, renderer->GetPipelineLayout(), renderer->texDescriptorSets[currentFrame], renderer->GetDefaultSampler());
	glm::mat4 modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f)) * glm::rotate(glm::mat4(1), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	plane.Draw(commandBuffer, renderer->GetPipelineLayout(), renderer->texDescriptorSets[currentFrame], renderer->GetDefaultSampler(),modelMat);
}
#pragma endregion

#pragma region Input Callbacks
void ProcessInput(GLFWwindow* window, float deltaTime) {
	CAMERA_MOVERMENT type = CAMERA_MOVERMENT::NONE;
	if (glfwGetKey(window, GLFW_KEY_W)) {
		type = CAMERA_MOVERMENT::FORWARD;
	}
	else if (glfwGetKey(window, GLFW_KEY_S)) {
		type = CAMERA_MOVERMENT::BACK;
	}
	else if (glfwGetKey(window, GLFW_KEY_A)) {
		type = CAMERA_MOVERMENT::LEFT;
	}
	else if (glfwGetKey(window, GLFW_KEY_D)) {
		type = CAMERA_MOVERMENT::RIGHT;
	}
	else if (glfwGetKey(window, GLFW_KEY_E)) {
		type = CAMERA_MOVERMENT::UP;
	}
	else if (glfwGetKey(window, GLFW_KEY_Q)) {
		type = CAMERA_MOVERMENT::DOWN;
	}
	mainCamera.ProcessKeyInput(type, deltaTime);
}
void mouse_Callback(GLFWwindow* window, double xPos_in, double yPos_in) {
	if (glfwGetMouseButton(window, 1) == GLFW_PRESS) {
		mainCamera.ProcessMouseMove(static_cast<float>(xPos_in), static_cast<float>(yPos_in));
	}
	else {
		mainCamera.firstMove = true;
	}
}
#pragma endregion

void Init() {
	if (!glfwInit()) {
		printf("Fail glfwInit\n");
		exit(EXIT_FAILURE);
	}
	//init window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //GLFW was originally designed to create an OpenGL context											  
												  //we need to tell it to not create an OpenGL context
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Engine", nullptr, nullptr);
	//set callback Func
	glfwSetCursorPosCallback(window, mouse_Callback);

	//set custom function
	RendererCustomFuncs funcs;
	funcs.checkSuitableDeviceFunc = isDeviceSuitable;
	funcs.setPhysicalDeviceFeaturesFunc = SetPhysicalDeviceFeatures;
	funcs.checkSwapSurfaceFormatFunc = CheckSwapSurfaceSupport;
	funcs.checkSwapPresentModeFunc = CheckSwapPresentMode;
	funcs.renderFunc = drawFunc;
	renderer = Renderer::GetInstance(window, &funcs);

}
void CreateShadowMap(int currentFrame, VkCommandBuffer CommandBuffer) {
	//render
	VkClearValue depthClear{};
	depthClear.depthStencil = { 1.0f, 0 };
	VkRenderPassBeginInfo renderPassInfo = Initializer::InitRenderPassBeginInfo(shadowMapRenderPass, shadowFramebuffer.GetCurrentFrameBuffer(currentFrame), { 0,0 }, shadowMap.textureSize, 1, &depthClear);
	vkCmdBeginRenderPass(CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	VkViewport viewport = Initializer::InitViewport(0.0f, 0.0f, shadowMap.textureSize.width, shadowMap.textureSize.height, 0.0f, 1.0f);
	vkCmdSetViewport(CommandBuffer, 0, 1, &viewport);
	VkRect2D Scissor = Initializer::InitScissor({ 0,0 }, shadowMap.textureSize);
	vkCmdSetScissor(CommandBuffer, 0, 1, &Scissor);
	vkCmdSetDepthBias(CommandBuffer, 1.25f, 0.0f, 1.75f);
	vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMapPipeline);
	vert_ubo.view = glm::lookAt(sun.direction, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	vert_ubo.proj = glm::ortho(0.0f, 1.0f, -0.5f, 0.5f, 0.1f, 100.0f);
	vert_ubo.proj[1][1] *= -1;
	vert_ubo.lightSpaceMat = vert_ubo.proj * vert_ubo.view;
	memcpy(ShadowVertexUniformBuffersMapped[currentFrame], &vert_ubo, sizeof(vert_ubo));
	vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMapPipeLayout, 0, 1, &shadowDescriptorSets[currentFrame], 0, nullptr);
	model.SetPosition(pos);
	model.Draw(CommandBuffer, renderer->GetPipelineLayout(), renderer->texDescriptorSets[currentFrame], renderer->GetDefaultSampler());
	model.SetPosition(pos + glm::vec3(0.2f,0.1f,0.2f));
	model.Draw(CommandBuffer, renderer->GetPipelineLayout(), renderer->texDescriptorSets[currentFrame], renderer->GetDefaultSampler());
	glm::mat4 modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f)) * glm::rotate(glm::mat4(1), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	plane.Draw(CommandBuffer, shadowMapPipeLayout, nullptr, nullptr, modelMat);
	vkCmdEndRenderPass(CommandBuffer);
}
void PrepareShadowMap() {
	//RenderPass
	PipelineBuilder::RenderPassCreateInfos infos{};
	VkFormat depthFormat = Utils::findDepthFormat(renderer->physicalDevice);
	VkAttachmentDescription depthAttachment = Initializer::InitAttachmentDescription(depthFormat, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_ATTACHMENT_STORE_OP_STORE);
	infos.attachmentDescriptors = { depthAttachment };

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 0;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	infos.subpasses.emplace_back();
	infos.subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	infos.subpasses[0].pDepthStencilAttachment = &depthAttachmentRef;

	infos.dependencies.emplace_back();

	
	infos.dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	infos.dependencies[0].dstSubpass = 0;
	infos.dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	infos.dependencies[0].srcAccessMask = 0;
	infos.dependencies[0].dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	infos.dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	PipelineBuilder::CreateRenderPass(shadowMapRenderPass, renderer->device, infos);

	//DescriptorSet
	DescriptorBuilder::CreateVertexUBO_DescriptorSets(renderer->device, shadowDescriptorSetLayout, shadowDescriptorPool, shadowDescriptorSets);
	Utils::CreateVertexUniforBuffer(renderer->device, renderer->physicalDevice, ShadowVertexUnifomrBuffer, ShadowVertexUniformBuffersMemory, ShadowVertexUniformBuffersMapped, MAX_FRAMES_IN_FLIGHT);
	std::vector<VkWriteDescriptorSet> writes;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo vertBufferInfo = Initializer::InitDescriptorBufferInfo(ShadowVertexUnifomrBuffer[i], sizeof(GlobalStructs::VertexShaderUBO));
		writes.push_back(Initializer::InitWriteDescriptorSet(shadowDescriptorSets[i], 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &vertBufferInfo));
	}
	vkUpdateDescriptorSets(renderer->device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

	vector<VkDescriptorSetLayout> desc_set = { shadowDescriptorSetLayout};
	//Pipeline
	PipelineBuilder::CreateGraphicsPipeline(shadowMapPipeline, shadowMapPipeLayout, renderer->device, "ShadowMappingVert.spv", "ShadowMappingFrag.spv", shadowMapRenderPass, desc_set);
	
	//Texture
	shadowMap.Create(2048.f, 2048.f, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	
	//FrameBuffer
	vector<VkImageView> attachments = { shadowMap.textureImageView };
	shadowFramebuffer.SetUp(shadowMap.textureSize, attachments, shadowMapRenderPass, MAX_FRAMES_IN_FLIGHT);

	VkSamplerCreateInfo createinfo = SamplerBuilder::InitSamplerCreateInfo();
	createinfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	createinfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	createinfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	createinfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	if (vkCreateSampler(renderer->device, &createinfo, nullptr, &shadowSampler)) {
		throw std::runtime_error("failed to create sampler!");
	}
}

int main()
{
	Init();

	model.LoadModel(renderer, "Assets/lubricant_spray_4k.gltf/lubricant_spray_4k.gltf");
	pos = mainCamera.position + 0.5f * mainCamera.Front;
	model.SetPosition(pos);
	plane = PrimitiveMesh::CreateQuad();
	plane.SetPosition(0.0, 0.0f, -0.5f);
	sun.direction = glm::vec3(10.0f, 10.0f, 10.0f);
	sun.intensity = 2.0f;
	frag_ubo.dirLight = sun;
	PrepareShadowMap();
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		float deltaTime = renderer->GetDeltaTime();
		//key input
		ProcessInput(window, deltaTime);
		//render
		renderer->Render();
	}

	vkDeviceWaitIdle(renderer->device);
	Clean();
	renderer->Clean();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

