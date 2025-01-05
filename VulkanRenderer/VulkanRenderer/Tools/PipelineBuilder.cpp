#include "PipelineBuilder.hpp"

VkShaderModule PipelineBuilder::CreateShaderModule(VkDevice device, const std::string& fn) {
	auto shaderCode = FileLoader::LoadShaderfile(fn);
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderCode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}
	return shaderModule;
}

void PipelineBuilder::CreateGraphicsPipeline(VkPipeline& out_pipeline, VkPipelineLayout& out_pipelineLayout, const VkDevice device, const std::string& vsFilename, const std::string& fsFilename, const VkRenderPass renderpass, std::vector<VkDescriptorSetLayout>& descriptorSetLayout, PipelineCreateInfos infos, uint32_t subpass) {
	VkShaderModule vertShaderModule = CreateShaderModule(device, vsFilename);
	VkShaderModule fragShaderModule = CreateShaderModule(device, fsFilename);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main"; // specify the function to invoke, known as entrypoint.
										// That means that it's possible to combine multiple fragment shader into 
										// single shader module and use different entry points to differentiate
										// differentiate between their behaviors/
	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	infos.shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = descriptorSetLayout.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayout.data();
	pipelineLayoutInfo.pushConstantRangeCount = infos.push_constant.size();
	pipelineLayoutInfo.pPushConstantRanges = infos.push_constant.data();

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &out_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout");
	}

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(infos.shaderStages.size());
	pipelineCreateInfo.pStages = infos.shaderStages.data();
	pipelineCreateInfo.pVertexInputState = &infos.vertexInputInfo;
	pipelineCreateInfo.pInputAssemblyState = &infos.inputAssembly;
	pipelineCreateInfo.pViewportState = &infos.viewportState;
	pipelineCreateInfo.pRasterizationState = &infos.rasterizer;
	pipelineCreateInfo.pMultisampleState = &infos.multisampling;
	pipelineCreateInfo.pDepthStencilState = &infos.depthStencil;
	pipelineCreateInfo.pColorBlendState = &infos.colorBlending;
	pipelineCreateInfo.pDynamicState = &infos.dynamicState;
	pipelineCreateInfo.layout = out_pipelineLayout;
	pipelineCreateInfo.renderPass = renderpass;
	pipelineCreateInfo.subpass = subpass;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = -1;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &out_pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphicsPipeline!");
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}
void PipelineBuilder::CreateRenderPass(VkRenderPass& out, VkDevice device, RenderPassCreateInfos& infos) {
	VkRenderPassCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = static_cast<uint32_t>(infos.attachmentDescriptors.size());
	createInfo.pAttachments = infos.attachmentDescriptors.data();
	createInfo.subpassCount = static_cast<uint32_t>(infos.subpasses.size());
	createInfo.pSubpasses = infos.subpasses.data();
	createInfo.dependencyCount = static_cast<uint32_t>(infos.dependencies.size());
	createInfo.pDependencies = infos.dependencies.data();
	if (vkCreateRenderPass(device, &createInfo, nullptr, &out) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass");
	}
}

void PipelineBuilder::CreateDefaultRenderPass(VkRenderPass& out, VkDevice device, VkPhysicalDevice physicalDevice, VkFormat swapChainFormat) {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapChainFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = Utils::findDepthFormat(physicalDevice);
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::vector<VkAttachmentDescription> attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &out) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass");
	}
}