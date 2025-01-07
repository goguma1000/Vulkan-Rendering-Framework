# Vulkan Rendering Framework
## 프로젝트 개요
Vulkan API를 이용한 렌더링 프레임워크 개발</br>
렌더링 관련 학습 시, 매번 초기 설정에 많은 시간을 소모하지 않고,</br> 
렌더링에만 집중할 수 있도록 하기 위해 개발한 프레임워크.</br>

### 현재 구현 된 Features

* Update-After-Bind descriptor
* Model loading using Assimp
* Texture maping
* Depth test
* Mipmap generate
* visualize texture
* shadow mapping

### 구현 및 수정할 Feature
* Buffer 할당 개선 
* 3D texture
* Model에 Tranform 추가
* UI 추가
* 텍스처마다 stbi_set_flip_vertically_on_load를 해야하는 지 판단 후 적용
</br>. . .

### 해결해야 할 오류
* 처음 1~2프레임에 그림이 그려지지 않음

## Demo
![Demo IMG](https://media.githubusercontent.com/media/goguma1000/Vulkan-Rendering-Framework/main/Readme_source/DemoIMG.PNG)

## Description
### main
main함수는 'VulkanRenderer.cpp'파일에 존재한다.</br>
~~~c++

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
~~~
Init 함수를 호출하여 Renderer의 custome function들을 설정하고 window 및 glfw를 초기화한다.</br>
funcs.renderFunc을 제외한 나머지 항목들은 renderer class를 Init할 때 사용하므로 GetInstance 함수를 호출하기 전에 설정해야 한다.</br>

**관련 코드 링크 :**</br>
[VulkanRenderer.cpp](https://github.com/goguma1000/Vulkan-Rendering-Framework/tree/main/VulkanRenderer/VulkanRenderer/VulkanRenderer.cpp)

### Render Function
main함수의 render Loop는 다음과 같다.
~~~c++
int main()
{
	. . .
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		renderer->Render();
	}
	. . .
}
~~~

~~~c++
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
~~~
Render함수에서는 화면에 그릴 swapchain image를 얻고, 이전에 설정한 render function을 호출한 후, </br>
record된 commandBuffer를 graphics queue에 제출하고, 그려진 image를 present queue에 제출하여 화면에 그린다.</br>

**관련 코드 링크 :**</br>
[Renderer.h](https://github.com/goguma1000/Vulkan-Rendering-Framework/tree/main/VulkanRenderer/VulkanRenderer/Renderer.h)</br>
[Renderer.cpp](https://github.com/goguma1000/Vulkan-Rendering-Framework/tree/main/VulkanRenderer/VulkanRenderer/Renderer.cpp)


### model load
~~~c++
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
~~~
Model class는 model에 사용된 texture와 mesh를 가지고 있다.</br>
LoadModel 함수를 통해서 model및 texture를 load할 수 있다.
~~~c++
// when you load 3d model
Model model;
model.LoadModel(renderer, filePath);
~~~ 
LoadModel 함수를 호출하려면 위와 같이 Model 변수를 미리 선언하고, 함수를 호출하면 된다.</br>
parameter로 renderer instance와 파일의 경로를 넣어주면 된다.</br>
</br>
Mesh class는 다음과 같다.
~~~c++
class Mesh {
public:
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
        ...
	}
private:
	std::vector<Vertex>			vertices;
	std::vector<unsigned int>	indices;
	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
	VkBuffer indexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    . . .
};
~~~
Mesh 는 material, vertex data, index data, 그리고 그들을 담은 buffer가 존재한다.</br>
material에는 load된 texture의 index를 가지고 있다.</br>
</br>

**관련 코드 링크 :**</br>
[Model.hpp](https://github.com/goguma1000/Vulkan-Rendering-Framework/tree/main/VulkanRenderer/VulkanRenderer/Model/Model.hpp)</br>
[Model.cpp](https://github.com/goguma1000/Vulkan-Rendering-Framework/tree/main/VulkanRenderer/VulkanRenderer/Model/Model.cpp)</br>

[Mesh.hpp](https://github.com/goguma1000/Vulkan-Rendering-Framework/tree/main/VulkanRenderer/VulkanRenderer/Model/Mesh.hpp)</br>
[Mesh.cpp](https://github.com/goguma1000/Vulkan-Rendering-Framework/tree/main/VulkanRenderer/VulkanRenderer/Model/Mesh.cpp)</br>

### Texture

Texture structure는 다음과 같다.
~~~c++
struct Texture{
	VkExtent2D textureSize = { 0,0 };
	uint32_t mipLevels = 1;
	VkImage textureImage = VK_NULL_HANDLE;
	VkImageView textureImageView = VK_NULL_HANDLE;
	VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
	string path = "";
public:
	Texture(const string& _path) :path(_path) {};

	void Load(const string& fn, bool sRGB = false, bool isHdr = false, bool genMipmap = true, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL) {
		Renderer* renderer = Renderer::GetInstance();
		if (renderer == nullptr) {
			std::cout << "renderer instance is nullptr! please create renderer instance  calling GetInstance(GlfwWindow, rendererCustomFuncs)!";
			return;
		}
		void* buf = nullptr;
		int width = 0, height = 0, nChannels = 0;
		//4채널이미지가 아니면 4채널로 만들어 줘야함 1채널과 3채널은 STBI_rgb_alpha를 쓰면 되는데 2채널은 따로 추가해줘야함
		stbi_set_flip_vertically_on_load(true);
		if (isHdr) {
			buf = (float*)stbi_loadf(fn.c_str(), &width, &height, &nChannels, STBI_rgb_alpha);
		}
		else {
			buf = (unsigned char*)stbi_load(fn.c_str(), &width, &height, &nChannels, STBI_rgb_alpha);
		}

		if (buf) {
			cout << fn << " image load success! width : " << width << " height : " << height << " channels : " << nChannels << std::endl;
		}
		else {
			throw std::runtime_error("failed to load texture image!");
		}
		. . . 
        //create texture image and mipmap
        //create texture imageview
	}
~~~
Texture struct는 texture의 miplevel과 texture image 및 view를 갖고있다.</br>
~~~c++
Texture texture(filePath);;
texture.Load(filePath, ...);
~~~
Texture loading은 위와 같이 texture 변수를 선언하고 Load함수를 호출하면 된다.</br>

#### texture 생성

~~~c++
void Create(float width, float height, VkFormat format, VkImageUsageFlags usages, VkImageAspectFlagBits flagBits, VkImageLayout nextLayout) {
		textureSize.width = width;
		textureSize.height = height;
		if (textureImage != VK_NULL_HANDLE) {
			Clean();
		}
		Renderer* instance = Renderer::GetInstance();
		if (instance == nullptr) {
			std::cout << "renderer instance is nullptr! please create renderer instance  calling GetInstance(GlfwWindow, rendererCustomFuncs)!";
			return;
		}
		VkImageCreateInfo imageInfo = Initializer::InitImageCreateInfo(VK_IMAGE_TYPE_2D, width, height, 1, 1, format, VK_IMAGE_TILING_OPTIMAL, usages);
		Utils::CreateImage(instance->device, instance->physicalDevice, textureImage, textureImageMemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, imageInfo);
		textureImageView = Utils::CreateImageView(instance->device, textureImage, format, VK_IMAGE_VIEW_TYPE_2D, flagBits);
		Utils::transitionImageLayout(instance->device, instance->commandPool, instance->graphicsQueue, textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED, nextLayout, 1);
	}
~~~
Shadow map과 같은 texture를 만들어야 할 때 Texture의 Create함수를 호출하여 texture를 생성한다.</br>
parameter로 texture의 크기와 format, usage, aspect, 최종 image layout을 설정한다.</br>

#### texture visualize
~~~c++
void Texture::Show(VkCommandBuffer commandBuffer, uint32_t currentFrame, VkImageLayout imgeLayout, VkSampler sampler) {
	if (PrimitiveMesh::quad.GetMeshCount() < 1) {
		PrimitiveMesh::CreateQuad();
	}
	Renderer* instance = Renderer::GetInstance();
	VkPipelineLayout pipelineLayout = instance->GetTextureDebugPipelineLayout();
	VkPipeline pipeline = instance->GetTextureDebugPipeline();
	VkDescriptorSet descriptorSet = instance->GetTextureDebugDescriptorSet(currentFrame);
	if (sampler == VK_NULL_HANDLE) {
		sampler = instance->GetDefaultSampler();
	}
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	VkDescriptorImageInfo imageInfo = Initializer::InitDescriptorImageInfo(imgeLayout, textureImageView, sampler);
	VkWriteDescriptorSet write = Initializer::InitWriteDescriptorSet(descriptorSet, 0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, nullptr, &imageInfo);
	vkUpdateDescriptorSets(instance->device, 1, &write, 0, nullptr);
	PrimitiveMesh::RenderQuad(commandBuffer,glm::mat4(1),instance->GetTextureDebugPipelineLayout());
}
~~~
Texture가 잘 load가 됐는지 또는 잘 생성이 됐는지 확인이 필요할 때가 있다.</br>
그 때 다음과 같이 render loop에서 texture의 show 함수를 호출하여 texture를 시각화 할 수 있다.</br>
~~~c++
void drawFunc(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, uint32_t currentFrame) {
	. . .
	texure.show(commandBuffer, currentFrame, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler);
}
~~~

**관련 코드 링크 :**</br>
[Texture.hpp](https://github.com/goguma1000/Vulkan-Rendering-Framework/tree/main/VulkanRenderer/VulkanRenderer/Model/Texture.hpp)</br>
[Texture.cpp](https://github.com/goguma1000/Vulkan-Rendering-Framework/tree/main/VulkanRenderer/VulkanRenderer/Model/Texture.cpp)</br>

### Framebuffer

FrameBuffer class는 다음과 같다.</br>
~~~c++
class FrameBuffer {
	std::vector<VkFramebuffer> framebuffers;
public:
	void Clean();
	void SetUp(VkExtent2D extent, std::vector<VkImageView>& attachments, VkRenderPass renderPass, const uint32_t frameCount = MAX_FRAMES_IN_FLIGHT);
	const VkFramebuffer GetCurrentFrameBuffer(int curFrame) const { return framebuffers[curFrame]; }
};
namespace Utils {
	void CreateFrameBuffer(VkFramebuffer& out, const VkDevice device, const std::vector<VkImageView>& attachments, const VkRenderPass renderpass, const VkExtent2D& extent);
}
~~~
FrameBuffer를 생성할 때 변수를 선언한 후 SetUp함수를 호출하면 된다.</br>
parameter에서 attachments에 texture할당해주면 shadow map과 같은 texture를 생성할 수 있다.</br>

**관련 코드 링크 :**</br>
[FrameBuffer.hpp](https://github.com/goguma1000/Vulkan-Rendering-Framework/tree/main/VulkanRenderer/VulkanRenderer/Tools/FrameBuffer.hpp)</br>
[FrameBuffer.cpp](https://github.com/goguma1000/Vulkan-Rendering-Framework/tree/main/VulkanRenderer/VulkanRenderer/Tools/FrameBuffer.cpp)</br>

### Draw model
model을 draw할 때는 custome render function에서 다음과 같이 코드를 작성하면 된다.</br>
~~~c++
void drawFunc(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, uint32_t currentFrame) {
	. . .
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
~~~

~~~c++
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
~~~
model class의 Draw 함수를 호출하여 draw할 수 있다.</br>
Model::Draw함수는 우선 load된 texture들로 descriptorset을 update한다.</br>
그 후, push constant를 사용하여 model matrix를 vertex shader에서 읽을 수 있게 한다.</br>
마지막으로 model을 이루고 있는 각 mesh의 Draw함수를 호출하여 model을 화면에 그린다.</br>

**관련 코드 링크 :**</br>
[Model.hpp](https://github.com/goguma1000/Vulkan-Rendering-Framework/tree/main/VulkanRenderer/VulkanRenderer/Model/Model.hpp)</br>
[Model.cpp](https://github.com/goguma1000/Vulkan-Rendering-Framework/tree/main/VulkanRenderer/VulkanRenderer/Model/Model.cpp)</br>

### Update-After-Bind descriptor
~~~c++
void DescriptorBuilder::CreateBindlessDescriptorSets(VkDevice device,VkDescriptorSetLayout& outLayout, VkDescriptorPool& outPool, std::vector<VkDescriptorSet>& outSets, const uint32_t setCount, const uint32_t descriptorCount) {
	VkDescriptorSetLayoutBinding set_binding = Initializer::InitDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorCount, VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayoutCreateInfo createInfo = Initializer::InitDescriptorSetLayoutCreateInfo(1,&set_binding);
    createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;

	const VkDescriptorBindingFlagsEXT flags =
		VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT |
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
		VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
		VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;
	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT binding_flags{};
	binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
	binding_flags.bindingCount = 1;
	binding_flags.pBindingFlags = &flags;

	createInfo.pNext = &binding_flags;
	if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &outLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout");
	};

	VkDescriptorPoolSize poolSize;
	poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize.descriptorCount = descriptorCount * setCount;
	VkDescriptorPoolCreateInfo poolCreateInfo = Initializer::InitDescriptorPoolCreateInfo(1, &poolSize, setCount);
	poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
	if (vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &outPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}

	std::vector<VkDescriptorSetLayout> layouts(setCount, outLayout);
	VkDescriptorSetAllocateInfo allocInfo = Initializer::InitDescriptorSetAllocateInfo(outPool, static_cast<uint32_t>(layouts.size()), layouts.data());
	VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variable_info{};
	variable_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
	variable_info.descriptorSetCount = setCount;
	variable_info.pDescriptorCounts = &descriptorCount;
	allocInfo.pNext = &variable_info;
	outSets.resize(setCount);
	if (vkAllocateDescriptorSets(device, &allocInfo, outSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}
}
~~~
model마다 사용하는 Texture가 다르기 때문에 각 모델을 draw할 때마다 texture descriptor set을 update해줘야 한다.</br>
하지만 한 번 descriptor set을 update하면 command queue가 제출되기 전까지 update를 할 수 없다.</br>
이러한 문제를 해결하기 위해 Update-After-Bind descriptor방식을 적용하였다.</br>
DescriptorSetLayout과 descriptor pool의 flag에 UPDATE_AFTER_BIND_BIT를 적용한다.</br>
VkDescriptorBindingFlagsEXT에서 각 EXT_BIT는 다음을 의미한다.</br>

* VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT : 각 Descripotr set에 대해 고정된 양의 descripotr를 할당할 필요가 없이 가변적인 양으로 사용할 수 있다.
* VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT : 모든 descripotr를 bind할 필요없이 shader에서 실제로 사용될 descriptor만 bind된다.
* VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT : Descriptor set이 command buffer에 bind되어도 update를 할 수 있도록 한다.
* VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT : command buffer가 실행되고 있는 동안 descriptor를 업데이트 할 수 있도록 해준다.

**관련 코드 링크 :**</br>
[PipelineBuilder.hpp](https://github.com/goguma1000/Vulkan-Rendering-Framework/blob/main/VulkanRenderer/VulkanRenderer/Tools/PipelineBuilder.hpp)</br>
[PipelineBuilder.cpp](https://github.com/goguma1000/Vulkan-Rendering-Framework/blob/main/VulkanRenderer/VulkanRenderer/Tools/PipelineBuilder.cpp)</br>




