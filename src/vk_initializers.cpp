#include <vk_initializers.h>

namespace vkinit {
	VkCommandPoolCreateInfo
		command_pool_create_info(uint32_t queueFamilyIndex,
			VkCommandPoolCreateFlags flags) {

		VkCommandPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.queueFamilyIndex = queueFamilyIndex;
		info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		return info;
	}

	VkCommandBufferAllocateInfo
		command_buffer_allocate_info(VkCommandPool command_pool, uint32_t count,
			VkCommandBufferLevel level) {

		VkCommandBufferAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandBufferCount = 1;
		allocateInfo.commandPool = command_pool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		return allocateInfo;
	}
	VkPipelineShaderStageCreateInfo
		pipeline_shader_stage_create_info(VkShaderStageFlagBits stage,
			VkShaderModule shader_module) {
		VkPipelineShaderStageCreateInfo info = {
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		info.stage = stage;
		info.module = shader_module;
		info.pName = "main";
		return info;
	}

	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info() {

		VkPipelineVertexInputStateCreateInfo info = {
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		info.vertexBindingDescriptionCount = 0;
		info.vertexAttributeDescriptionCount = 0;
		return info;
	}

	VkPipelineInputAssemblyStateCreateInfo
		input_assembly_create_info(VkPrimitiveTopology topology) {

		VkPipelineInputAssemblyStateCreateInfo info = {
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		info.topology = topology;
		info.primitiveRestartEnable = VK_FALSE;
		return info;
	}

	VkPipelineRasterizationStateCreateInfo
		rasterization_state_create_info(VkPolygonMode polygonMode) {
		VkPipelineRasterizationStateCreateInfo info = {
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		info.cullMode = VK_CULL_MODE_NONE;

		info.depthBiasClamp = 0.0f;
		info.depthBiasConstantFactor = 0.0f;
		info.depthBiasEnable = VK_FALSE;
		info.depthBiasSlopeFactor = 0.0f;
		info.depthClampEnable = VK_FALSE;

		info.frontFace = VK_FRONT_FACE_CLOCKWISE;

		info.lineWidth = 1.0f;

		info.polygonMode = polygonMode;
		info.rasterizerDiscardEnable = VK_FALSE;

		return info;
	}

	VkPipelineMultisampleStateCreateInfo multisampling_state_create_info() {

		VkPipelineMultisampleStateCreateInfo info = {
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		info.sampleShadingEnable = VK_FALSE;

		info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		info.minSampleShading = 1.0f;

		info.alphaToCoverageEnable = VK_FALSE;
		info.alphaToOneEnable = VK_FALSE;

		return info;
	}

	VkPipelineColorBlendAttachmentState color_blend_attachment_state() {
		VkPipelineColorBlendAttachmentState info = {};
		info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		info.blendEnable = VK_FALSE;

		return info;
	}

	VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass) {

		VkPipelineViewportStateCreateInfo viewportState = {
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		viewportState.viewportCount = 1;
		viewportState.pViewports = &m_viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &m_scissor;

		VkPipelineColorBlendStateCreateInfo colorBlending = {};

		colorBlending.sType =
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &m_colorBlendAttachment;

		// building the pipeline

		VkGraphicsPipelineCreateInfo pipelineInfo = {
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

		pipelineInfo.stageCount = static_cast<uint32_t>(m_shaderStages.size());
		pipelineInfo.pStages = m_shaderStages.data();

		pipelineInfo.pVertexInputState = &m_vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &m_inputAssembly;

		pipelineInfo.pViewportState = &viewportState;

		pipelineInfo.pRasterizationState = &m_rasterizer;
		pipelineInfo.pMultisampleState = &m_multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDepthStencilState = &m_depth_stencil;

		pipelineInfo.layout = m_pipelineLayout;

		pipelineInfo.renderPass = pass;
		pipelineInfo.subpass = 0;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		VkPipeline pipelineNew;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
			nullptr, &pipelineNew) != VK_SUCCESS) {
			printf("failed to create pipeline!\n");
			return VK_NULL_HANDLE;
		}

		return pipelineNew;
	}

	VkPipelineLayoutCreateInfo pipeline_layout_create_info() {

		VkPipelineLayoutCreateInfo info = {
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

		info.flags = 0;
		info.setLayoutCount = 0;
		info.pushConstantRangeCount = 0;

		return info;
	}

	VkImageCreateInfo image_create_info(VkFormat format,
		VkImageUsageFlags usageFlags,
		VkExtent3D extent) {
		VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		info.imageType = VK_IMAGE_TYPE_2D;
		info.extent = extent;
		info.format = format;
		info.mipLevels = 1;
		info.arrayLayers = 1;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = usageFlags;

		return info;
	}

	VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image,
		VkImageAspectFlags aspectFlags) {
		VkImageViewCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		info.format = format;
		info.image = image;
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.subresourceRange.aspectMask = aspectFlags;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.layerCount = 1;
		return info;
	}

	VkPipelineDepthStencilStateCreateInfo
		depth_stencil_create_info(bool depth_test, bool depth_write,
			VkCompareOp compareOp) {

		VkPipelineDepthStencilStateCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		info.pNext = nullptr;

		info.depthTestEnable = depth_test ? VK_TRUE : VK_FALSE;
		info.depthWriteEnable = depth_write ? VK_TRUE : VK_FALSE;
		info.depthCompareOp = depth_test ? compareOp : VK_COMPARE_OP_ALWAYS;
		info.depthBoundsTestEnable = VK_FALSE;
		info.minDepthBounds = 0.0f;
		info.maxDepthBounds = 1.0f;
		info.stencilTestEnable = VK_FALSE;

		return info;
	}


	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding)
	{
		VkDescriptorSetLayoutBinding b = {};
		b.binding = binding;
		b.descriptorCount = 1;
		b.descriptorType = type;
		b.stageFlags = stageFlags;
		return b;
	}

	VkWriteDescriptorSet writeDescriptorBuffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding)
	{
		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = nullptr;

		write.dstBinding = binding;
		write.dstSet = dstSet;
		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pBufferInfo = bufferInfo;

		return write;
	}



	VkSamplerCreateInfo samplerCreateInfo(VkFilter filters, VkSamplerAddressMode samplerAdressMode ) {
		VkSamplerCreateInfo info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
		info.magFilter = filters; 
		info.minFilter = filters; 
		info.addressModeU = samplerAdressMode;
		info.addressModeV = samplerAdressMode;
		info.addressModeW = samplerAdressMode;

		return info;
	}
	VkWriteDescriptorSet writeDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding) {
	
		VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		write.dstBinding = binding;
		write.dstSet = dstSet;;
		write.descriptorCount = 1;
		write.descriptorType = type;;
		write.pImageInfo = imageInfo;
	
		return write; 
	}



} // namespace vkinit
