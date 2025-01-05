#pragma once

#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include<vector>
#include <stdexcept>
#include <functional>
#include "Tools/Utils.hpp"
#include "GlobalStructs.hpp"

struct RendererCustomFuncs {
	std::function<bool(VkPhysicalDevice device)> checkSuitableDeviceFunc = nullptr;
	std::function<void(VkPhysicalDeviceFeatures& deviceFeatures)> setPhysicalDeviceFeaturesFunc = nullptr;
	std::function<bool(const VkSurfaceFormatKHR& availableFormat)>checkSwapSurfaceFormatFunc = nullptr;
	std::function<bool(const VkPresentModeKHR& availableFormat)>checkSwapPresentModeFunc = nullptr;
	std::function<void(VkCommandBuffer, VkFramebuffer, uint32_t)> renderFunc = nullptr;
};
const int MAX_FRAMES_IN_FLIGHT = 2;
const int MAX_NUM_TEXTURE_BINDING = 8;

class Renderer {

public:
	std::function<bool(VkPhysicalDevice device)> checkSuitableDeviceFunc = [](VkPhysicalDevice device) {return false;};
	std::function<void(VkPhysicalDeviceFeatures& deviceFeatures)> setPhysicalDeviceFeaturesFunc = [](VkPhysicalDeviceFeatures& deviceFeatures) {};
	std::function<bool(const VkSurfaceFormatKHR& availableFormat)>checkSwapSurfaceFormatFunc = nullptr;
	std::function<bool(const VkPresentModeKHR& availableFormat)>checkSwapPresentModeFunc = nullptr;
	const std::vector<const char*> deviceExtension = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		VK_KHR_MAINTENANCE3_EXTENSION_NAME
	};
	std::function<void(VkCommandBuffer, VkFramebuffer, uint32_t)> renderFunc = nullptr;

public:
	VkPhysicalDevice physicalDevice = { VK_NULL_HANDLE };
	VkDevice device = { VK_NULL_HANDLE };
	VkQueue graphicsQueue = { VK_NULL_HANDLE };
	VkQueue presentQueue = { VK_NULL_HANDLE };
	VkCommandPool commandPool = { VK_NULL_HANDLE };
	std::vector<VkDescriptorSet>texDescriptorSets;
	VkDescriptorSetLayout texDescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool texDescriptorPool = VK_NULL_HANDLE;
private:
#ifdef NDEBUG
	const bool enableValidationLayer = false;
#else
	const bool enableValidationLayer = true;
#endif
	static Renderer* rendererInstance;

	static bool isInitialized;
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
	GLFWwindow* window = nullptr;
	VkInstance instance = {VK_NULL_HANDLE};
	VkDebugUtilsMessengerEXT debugMessenger = { VK_NULL_HANDLE };
	VkSurfaceKHR surface = { VK_NULL_HANDLE };
	VkSwapchainKHR swapChain = { VK_NULL_HANDLE };
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
	VkExtent2D swapChainExtent = {0,0};
	std::vector<VkImageView> swapChainImageViews;
	VkRenderPass defaultRenderpass = { VK_NULL_HANDLE };
	VkDescriptorSetLayout defaultDescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet>descriptorSets;
	VkSampler defaultSampler = VK_NULL_HANDLE;
	VkPipeline defaultPipeline = { VK_NULL_HANDLE };
	VkPipelineLayout defaultPipelineLayout = { VK_NULL_HANDLE };
	VkImage depthImage = VK_NULL_HANDLE;
	VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
	VkImageView depthImageView = VK_NULL_HANDLE;

	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkCommandBuffer> commandBuffers;
	
	std::vector<VkBuffer> vertexUniformBuffers;
	std::vector<VkDeviceMemory> vertexUniformBuffersMemory;
	std::vector<void*> vertexUniformBuffersMapped;

	std::vector<VkBuffer> fragUniformBuffers;
	std::vector<VkDeviceMemory> fragUniformBuffersMemory;
	std::vector<void*> fragUniformBuffersMapped;
	
	VkPipeline textureDebugPipeline;
	VkPipelineLayout textureDebugPipelineLayout;
	VkDescriptorSetLayout textureDebugDescriptorSetLayout;
	VkDescriptorPool textureDebugDescriptorPool;
	std::vector<VkDescriptorSet> textureDebugDescriptorSets;

	uint32_t currentFrame = 0;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	bool framebufferResized = false;
	float deltaTime = 0.0f;
	float lastTime = 0.0f;
public:
	void Init();
	void Render();
	void Clean();
	static Renderer* GetInstance();
	static Renderer* GetInstance(GLFWwindow* window, RendererCustomFuncs* funcs);
	void UpdateVertexUniformBuffer(uint32_t currentImage, GlobalStructs::VertexShaderUBO& ubo);
	void UpdateFragUniformBuffer(uint32_t currentImage, GlobalStructs::FragmentShaderUBO& ubo);

#pragma region Getter Functions
	//Gettter Functions
	const VkPipeline GetPipeline() const { return  defaultPipeline; }
	const VkPipelineLayout GetPipelineLayout() const { return defaultPipelineLayout; }
	const VkRenderPass GetRenderPass() const { return defaultRenderpass; }
	const VkExtent2D GetSwapChainExtent() const { return swapChainExtent; }
	const VkDescriptorSet GetDescriptorSet(uint32_t currentFrame) const { return isInitialized ? descriptorSets[currentFrame] : VK_NULL_HANDLE; }
	const VkSampler GetDefaultSampler() const { return defaultSampler; }
	const VkDescriptorSetLayout GetDefaultDescriptorSetLayout() const { return defaultDescriptorSetLayout; }
	const VkBuffer GetVertexUniformBuffer(uint32_t currentFrame) const { return vertexUniformBuffers[currentFrame]; }
	const VkBuffer GetFragUniformBuffer(uint32_t currentFrame) const { return fragUniformBuffers[currentFrame]; }
	const VkDescriptorSetLayout GetTextureDebugDescriptorSetLayout() const { return textureDebugDescriptorSetLayout; }
	const VkPipelineLayout GetTextureDebugPipelineLayout() const{ return textureDebugPipelineLayout; };
	const VkPipeline GetTextureDebugPipeline() const { return textureDebugPipeline; }
	const VkDescriptorSet GetTextureDebugDescriptorSet(uint32_t currentFrame) const { return textureDebugDescriptorSets[currentFrame]; }
	const float GetDeltaTime() { 
		float curTime = glfwGetTime();
		deltaTime = curTime - lastTime;
		lastTime = curTime;
		return deltaTime; 
	}
#pragma endregion

private:
	//block copy constructor, assignment opperation for singleton pattern. 
	Renderer(GLFWwindow* wd, RendererCustomFuncs* funcs);
	Renderer& operator=(const Renderer& rhs) = delete;
	Renderer(const Renderer& rhs) = delete;
	~Renderer() { 
		//Clean();
	};

	void CreateVKinstance();
	void SetupDebugMessenger();
	void CreateSurface();
	void PickFirstPhysicalDevice();
	void CreateLogicalDevice();
	void CreateSwapChain();
	void CreateDefaultDescriptorSetLayout();
	void CreateUniforBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	void CreateDepthResources();
	void CreateImageViews();
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSyncObject();
	void CreateDefaultSampler();
	void CreateTextureDebugResources();

	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool IsDeviceSuitable(VkPhysicalDevice device);
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	void CleanUpSwapChain();
	void RecreateSwapChain();
private:
	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
};