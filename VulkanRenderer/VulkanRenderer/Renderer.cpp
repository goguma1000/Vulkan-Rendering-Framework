#include "Renderer.h"
#include "Tools/PipelineBuilder.hpp"
#include "Tools/SamplerBuilder.hpp"
#include <Tools/FrameBuffer.hpp>
#include <cstdint>
#include <set>
#include <limits>
#include<algorithm>
#include <array>
#include <Tools/DescriptorBuilder.hpp>

using namespace Utils;
Renderer* Renderer::rendererInstance = nullptr;
bool Renderer::isInitialized = false;

Renderer::Renderer(GLFWwindow* wd, RendererCustomFuncs* funcs) : window(wd) {
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
	if (funcs->checkSuitableDeviceFunc != nullptr) checkSuitableDeviceFunc = funcs->checkSuitableDeviceFunc;
	if (funcs->setPhysicalDeviceFeaturesFunc != nullptr) setPhysicalDeviceFeaturesFunc = funcs->setPhysicalDeviceFeaturesFunc;
	checkSwapPresentModeFunc = funcs->checkSwapPresentModeFunc;
	checkSwapSurfaceFormatFunc = funcs->checkSwapSurfaceFormatFunc;
	renderFunc = funcs->renderFunc;
	Init();
	if (rendererInstance == nullptr) {
		rendererInstance = this;
	}
}
Renderer* Renderer::GetInstance() {
	if (rendererInstance == nullptr) {
		std::cout << "Please create Renderer instance!\n";
		return nullptr;
	}
	else if (!isInitialized) {
		std::cout << "Please call Init() func in main(), before doing your work!\n";
		return nullptr;
	}
	else return rendererInstance;
}

Renderer* Renderer::GetInstance(GLFWwindow* window, RendererCustomFuncs* funcs) {
	if (rendererInstance == nullptr) {
		rendererInstance = new Renderer(window, funcs);
	}
	else return rendererInstance;
}
void Renderer::Init() {
	CreateVKinstance();
	SetupDebugMessenger();
	CreateSurface();
	PickFirstPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapChain();
	CreateImageViews();
	PipelineBuilder::CreateDefaultRenderPass(defaultRenderpass, device, physicalDevice, swapChainImageFormat);
	CreateDefaultDescriptorSetLayout();
	CreateUniforBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	DescriptorBuilder::CreateBindlessDescriptorSets(device, texDescriptorSetLayout, texDescriptorPool, texDescriptorSets, MAX_FRAMES_IN_FLIGHT);
	CreateDefaultSampler();
	std::vector<VkDescriptorSetLayout> desc_layouts = { defaultDescriptorSetLayout,texDescriptorSetLayout };
	PipelineBuilder::CreateGraphicsPipeline(defaultPipeline, defaultPipelineLayout, device, "DefaultVertexShader.spv", "DefaultFragmentShader.spv", defaultRenderpass, desc_layouts);
	CreateCommandPool();
	CreateDepthResources();
	CreateFramebuffers();
	CreateCommandBuffers();
	CreateSyncObject();
	CreateTextureDebugResources();
	isInitialized = true;
}
void Renderer::Clean() {
	CleanUpSwapChain();
	vkDestroySampler(device, defaultSampler, nullptr);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroyBuffer(device, vertexUniformBuffers[i], nullptr);
		vkDestroyBuffer(device, fragUniformBuffers[i], nullptr);
		vkFreeMemory(device, vertexUniformBuffersMemory[i], nullptr);
		vkFreeMemory(device, fragUniformBuffersMemory[i], nullptr);
	}
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(device, defaultDescriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, texDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(device, texDescriptorSetLayout,nullptr);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}
	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroyPipeline(device, defaultPipeline, nullptr);
	vkDestroyPipelineLayout(device, defaultPipelineLayout, nullptr);
	vkDestroyRenderPass(device, defaultRenderpass, nullptr);

	vkDestroyDescriptorPool(device, textureDebugDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(device, textureDebugDescriptorSetLayout, nullptr);
	vkDestroyPipelineLayout(device, textureDebugPipelineLayout, nullptr);
	vkDestroyPipeline(device, textureDebugPipeline, nullptr);


	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	if (enableValidationLayer) {
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}
	vkDestroyInstance(instance, nullptr);
}

void Renderer::CreateVKinstance() {
	VkApplicationInfo appInfo{};
	appInfo.sType= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "DonghoRenderer";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "DonghoEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;
	
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	std::vector<const char*> extensions = GetRequiredExtension(enableValidationLayer);
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	
	if (enableValidationLayer && !CheckValidationLayerSupport(validationLayers)) {
		throw std::runtime_error("validation layers requested, but not available.");
	}
	
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (enableValidationLayer) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}
}

void Renderer::SetupDebugMessenger() {
	if (!enableValidationLayer) return;
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo);
	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void Renderer::CreateSurface() {
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

bool Renderer::checkDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtension.begin(), deviceExtension.end());
	std::cout << "Device extensions: \n";
	for (const auto& extension : availableExtensions) {
		std::cout << extension.extensionName << std::endl;
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

bool Renderer::IsDeviceSuitable(VkPhysicalDevice device) {
	QueueFamilyIndices temp = FindQueueFamiles(device, surface);
	bool extensionSupported = checkDeviceExtensionSupport(device);
	//after surface crate
	bool swapChainAdequate = false;
	if (extensionSupported) {
		SwapChainSupportDetails swapChainSupport = QuerrySwapChainSupport(device,surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}
	return temp.isComplete()&& extensionSupported && swapChainAdequate && checkSuitableDeviceFunc(device); 
}

void Renderer::PickFirstPhysicalDevice() {
	uint32_t deviceCount(0);
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	std::vector<VkPhysicalDevice>devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	for (const auto& device : devices) {
		if (IsDeviceSuitable(device)) {
			physicalDevice = device;
			//indices = FindQueueFamiles(physicalDevice, surface);
			break;
		}
	}
	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

void Renderer::CreateLogicalDevice() {
	QueueFamilyIndices indices = FindQueueFamiles(physicalDevice, surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamiles = { indices.graphicsFamily.value(), indices.presentFamily.value() };
	float queuePriority(1.0f);
	for (uint32_t queueFamily : uniqueQueueFamiles) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority; // influence the scheduling of command buffer execution
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{  };
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures{};
	indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
	indexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	indexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
	indexingFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
	indexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
	indexingFeatures.runtimeDescriptorArray = VK_TRUE;
	
	setPhysicalDeviceFeaturesFunc(deviceFeatures);
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtension.size());
	createInfo.ppEnabledExtensionNames = deviceExtension.data();
	createInfo.pNext = &indexingFeatures;
	if (enableValidationLayer) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else createInfo.enabledLayerCount = 0;

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}
	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue); //write 2024-08-15__03:10
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue); //write 2024-08-15__03:56.
	//In case the queue family are the same, two handles will most likely have the same value now.
}

VkSurfaceFormatKHR Renderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	bool isAvailable(false);
	for (const auto& availableFormat : availableFormats) {
		if (checkSwapSurfaceFormatFunc != nullptr) {
			if (checkSwapSurfaceFormatFunc(availableFormat)) isAvailable = true;
		}
		else if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			isAvailable = true;
		}
		if (isAvailable) return availableFormat;
	}
	return availableFormats[0];
}

VkPresentModeKHR Renderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	bool isAvailable(false);
	for (const auto& availablePresentMode : availablePresentModes) {
		if (checkSwapPresentModeFunc != nullptr) {
			if (checkSwapPresentModeFunc(availablePresentMode)) isAvailable = true;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			isAvailable = true;
		}
		if (isAvailable) return availablePresentMode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D Renderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	
	if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max())) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.width, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}
void Renderer::CreateSwapChain() {
	SwapChainSupportDetails swapChainSupport = QuerrySwapChainSupport(physicalDevice, surface);
	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = FindQueueFamiles(physicalDevice, surface);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}
	createInfo.presentMode = presentMode;
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void Renderer::CreateImageViews() {
	swapChainImageViews.resize(swapChainImages.size());
	for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
		swapChainImageViews[i] =
			CreateImageView(
				device,
				swapChainImages[i],
				swapChainImageFormat,
				VK_IMAGE_VIEW_TYPE_2D,
				VK_IMAGE_ASPECT_COLOR_BIT,
				1
			);
	}
}

//custom yourself. if you add Descriptorset
void Renderer::CreateDefaultDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding vert_uboLayoutBinding = Initializer::InitDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
	VkDescriptorSetLayoutBinding frag_uboLayoutBinding = Initializer::InitDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayoutBinding samplerLayoutBinding;
	std::vector<VkDescriptorSetLayoutBinding> bindings = { vert_uboLayoutBinding, frag_uboLayoutBinding };
	for (int i = 0; i < MAX_NUM_TEXTURE_BINDING; i++) {
		samplerLayoutBinding = Initializer::InitDescriptorSetLayoutBinding(bindings.size(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
		bindings.push_back(samplerLayoutBinding);
	}
	//re-write after create sampler
	VkDescriptorSetLayoutCreateInfo layoutInfo = Initializer::InitDescriptorSetLayoutCreateInfo(static_cast<uint32_t>(bindings.size()), bindings.data());
	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &defaultDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout");
	}
}

//custom yourself. if you add Descriptorset
void Renderer::CreateDescriptorPool() {
	std::vector<VkDescriptorPoolSize> poolSizes(3);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * GL_MAX_TEXTURE_SIZE;
	
	VkDescriptorPoolCreateInfo poolInfo = Initializer::InitDescriptorPoolCreateInfo(static_cast<uint32_t>(poolSizes.size()),poolSizes.data(), static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));	
	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

//custom yourself. if you add Descriptorset
void Renderer::CreateDescriptorSets() {
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, defaultDescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = Initializer::InitDescriptorSetAllocateInfo(descriptorPool,static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),layouts.data());
	descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}
	std::vector<VkWriteDescriptorSet> descriptorWrites;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo vertBufferInfo = Initializer::InitDescriptorBufferInfo(vertexUniformBuffers[i], sizeof(GlobalStructs::VertexShaderUBO));
		descriptorWrites.push_back(Initializer::InitWriteDescriptorSet(descriptorSets[i], 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &vertBufferInfo));
		VkDescriptorBufferInfo fragBufferInfo = Initializer::InitDescriptorBufferInfo(fragUniformBuffers[i], sizeof(GlobalStructs::FragmentShaderUBO));
		descriptorWrites.push_back(Initializer::InitWriteDescriptorSet(descriptorSets[i], 1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &fragBufferInfo));
	}
	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Renderer::CreateUniforBuffers() {
	VkDeviceSize buffersize = sizeof(GlobalStructs::VertexShaderUBO);
	vertexUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	vertexUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
	vertexUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		CreateBuffer(device, physicalDevice, buffersize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexUniformBuffers[i], vertexUniformBuffersMemory[i]);
		vkMapMemory(device, vertexUniformBuffersMemory[i], 0, buffersize, 0, &vertexUniformBuffersMapped[i]); //persistent mapping
	}

	buffersize = sizeof(GlobalStructs::FragmentShaderUBO);
	fragUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	fragUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
	fragUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		CreateBuffer(device, physicalDevice, buffersize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, fragUniformBuffers[i], fragUniformBuffersMemory[i]);
		vkMapMemory(device, fragUniformBuffersMemory[i], 0, buffersize, 0, &fragUniformBuffersMapped[i]); //persistent mapping
	}
}

void Renderer::CreateDefaultSampler() {
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);
	//max mipmap 8k image
	VkSamplerCreateInfo samplerInfo = SamplerBuilder::InitSamplerCreateInfo(14.0f, 0.0f, 0.0f, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_TRUE, properties.limits.maxSamplerAnisotropy);
	SamplerBuilder::CreateSampler(device, defaultSampler, samplerInfo);
}

void Renderer::CreateDepthResources() {
	VkFormat depthFormat = findDepthFormat(physicalDevice);
	VkImageCreateInfo imageInfo =  Initializer::InitImageCreateInfo(VK_IMAGE_TYPE_2D, swapChainExtent.width, swapChainExtent.height, 1, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	CreateImage(device, physicalDevice, depthImage, depthImageMemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, imageInfo);
	depthImageView = CreateImageView(device, depthImage, depthFormat, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT,1);
	transitionImageLayout(device, commandPool, graphicsQueue, depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

void Renderer::CreateFramebuffers() {
	swapChainFramebuffers.resize(swapChainImageViews.size());
	for (int i = 0; i < swapChainFramebuffers.size(); i++) {
		std::vector<VkImageView> attachments = { swapChainImageViews[i], depthImageView};
		CreateFrameBuffer(swapChainFramebuffers[i], device, attachments, defaultRenderpass, swapChainExtent);
	}
}

void Renderer::CreateCommandPool() {
	QueueFamilyIndices queueFamilyIndices = FindQueueFamiles(physicalDevice, surface);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}
void Renderer::CreateCommandBuffers() {
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command buffers!");
	}
}

void Renderer::CreateSyncObject() {
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = Initializer::InitSemaphoreCreateInfo();
	// On first frame, which immediately waits on inFlightFence to be signaled.
	// inFlightFence is only signaled after a frame has finished rendering
	// ,yet since this is the first frame, so vkWaitForFence blocks indefinitly.
	// one ofe the many solutions is create fence in the signaled state.
	VkFenceCreateInfo fenceInfo = Initializer::InitFenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS
			||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS
			||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create semaphores!");
		}
	}
}

void Renderer::CreateTextureDebugResources() {
	//파이프라인, descriptorset
	VkDescriptorSetLayoutBinding binding = Initializer::InitDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayoutCreateInfo createInfo = Initializer::InitDescriptorSetLayoutCreateInfo(1, &binding);
	if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &textureDebugDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout");
	}
	VkDescriptorPoolSize poolSize;
	poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);;
	VkDescriptorPoolCreateInfo poolCreateInfo = Initializer::InitDescriptorPoolCreateInfo(1, &poolSize, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));
	if (vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &textureDebugDescriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, textureDebugDescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = Initializer::InitDescriptorSetAllocateInfo(textureDebugDescriptorPool, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT), layouts.data());
	textureDebugDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(device, &allocInfo, textureDebugDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}
	std::vector<VkDescriptorSetLayout> desc_layout = { textureDebugDescriptorSetLayout };
	PipelineBuilder::CreateGraphicsPipeline(textureDebugPipeline, textureDebugPipelineLayout, device, "TextureDebugVert.spv", "TextureDebugFrag.spv", defaultRenderpass, desc_layout);//pipeline

}

void Renderer::Render() {

	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIdx;
	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIdx);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		RecreateSwapChain();
		std::cout << "Out of Time!\n";
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}
	vkResetFences(device, 1, &inFlightFences[currentFrame]); //Delay resetting the fence until after we know for sure we will be submitting work with it.

	vkResetCommandBuffer(commandBuffers[currentFrame], 0);
	VkCommandBufferBeginInfo beginInfo = Initializer::InitCommandBufferBeginInfo();
	//beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	VkResult res = vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed  to begin recording command buffer!");
	}

	renderFunc(commandBuffers[currentFrame],swapChainFramebuffers[imageIdx],currentFrame);
	
	vkCmdEndRenderPass(commandBuffers[currentFrame]);
	if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStage[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	VkSubmitInfo submitInfo = Initializer::InitSubmitInfo(1, waitSemaphores,waitStage,1, &commandBuffers[currentFrame], 1, signalSemaphores);
	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}
	VkPresentInfoKHR presentInfo = Initializer::InitPresentInfo(1, signalSemaphores, 1, &swapChain, &imageIdx);
	result = vkQueuePresentKHR(presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		RecreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}
	vkQueueWaitIdle(presentQueue);
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::CleanUpSwapChain() {
	vkDestroyImageView(device, depthImageView, nullptr);
	vkDestroyImage(device, depthImage, nullptr);
	vkFreeMemory(device, depthImageMemory, nullptr);
	for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
		vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
	}
	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		vkDestroyImageView(device, swapChainImageViews[i], nullptr);
	}
	vkDestroySwapchainKHR(device, swapChain, nullptr);
}
void Renderer::RecreateSwapChain() {
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}// when minimizing window
	vkDeviceWaitIdle(device);

	CleanUpSwapChain();

	CreateSwapChain();
	CreateImageViews();
	CreateDepthResources();
	CreateFramebuffers();
}

void Renderer::UpdateVertexUniformBuffer(uint32_t currentFrame, GlobalStructs::VertexShaderUBO& ubo) {
	memcpy(vertexUniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

void Renderer::UpdateFragUniformBuffer(uint32_t currentFrame, GlobalStructs::FragmentShaderUBO& ubo) {
	memcpy(fragUniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

#pragma region callback Function
void Renderer::FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}
#pragma endregion