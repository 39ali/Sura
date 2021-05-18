#include "vk_engine.h"
#pragma warning(disable : 26812)
#include <SDL.h>
#include <SDL_vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include "vk_initializers.h"
#include "vk_types.h"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "Tracy.hpp"
#include "common/TracySystem.hpp"





void VulkanEngine::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& func) {

	VkCommandBuffer cmd;
	VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandPool = m_gpuUploadConxtext.commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;
	VK_CHECK(vkAllocateCommandBuffers(m_device, &allocateInfo, &cmd));


	//begin command buffer recording 
	VkCommandBufferBeginInfo cmdBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
	//execute whatever 
	func(cmd);
	VK_CHECK(vkEndCommandBuffer(cmd));


	VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmd;

	VK_CHECK(vkQueueSubmit(m_graphics_queue, 1, &submit, m_gpuUploadConxtext.uploadFence));

	VK_CHECK(vkWaitForFences(m_device, 1, &m_gpuUploadConxtext.uploadFence, true, UINT64_MAX));
	VK_CHECK(vkResetFences(m_device, 1, &m_gpuUploadConxtext.uploadFence));

	VK_CHECK(vkResetCommandPool(m_device, m_gpuUploadConxtext.commandPool, 0));
}



void VulkanEngine::init() {
	ZoneScopedNS("int", 40);
	;

	// We initialize SDL and create a window with it.
	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

	m_window = SDL_CreateWindow("Vulkan Engine", SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED, m_windowExtent.width,
		m_windowExtent.height, window_flags);

	init_vulkan();
	init_swapchain();
	init_commands();
	init_render_pass();
	init_framebuffers();
	init_sync_structs();

	init_descriptors();

	init_pipelines();

	load_meshes();

	load_images();
	init_scene();

	tracy::SetThreadName("main Thread");

	// everything went fine
	m_isInitialized = true;
}

void VulkanEngine::init_vulkan() {
	ZoneScopedS(4);
	vkb::InstanceBuilder builder;

	auto inst = builder.set_app_name("Vulkan")
		.request_validation_layers(true)
		.require_api_version(1, 1, 0)
		.use_default_debug_messenger()
		.build();

	vkb::Instance vkb_inst = inst.value();

	m_instance = vkb_inst.instance;
	m_debug_messenger = vkb_inst.debug_messenger;

	SDL_Vulkan_CreateSurface(m_window, m_instance, &m_surface);

	vkb::PhysicalDeviceSelector selector{ vkb_inst };

	//
	auto physicalDeviceResult =
		selector.set_minimum_version(1, 1).set_surface(m_surface).select();
	vkb::PhysicalDevice physicalDevice;
	if (physicalDeviceResult.has_value()) {
		physicalDevice = physicalDeviceResult.value();
		printf("Using vulkan : 1.1\n");
	}
	else {
		auto physicalDeviceResult =
			selector.set_minimum_version(1, 0).set_surface(m_surface).select();
		physicalDevice = physicalDeviceResult.value();
		printf("Using vulkan : 1.0\n");
	}

	m_physical_device = physicalDevice.physical_device;

	m_physicalDeviceProperties = physicalDevice.properties;
	printf("GPU: %s \n ", m_physicalDeviceProperties.deviceName);
	printf("minimum buffer alignment %d \n", m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);

	vkb::DeviceBuilder deviceBuilder{ physicalDevice };
	vkb::Device device = deviceBuilder.build().value();
	m_device = device.device;

	m_graphics_queue_family =
		device.get_queue_index(vkb::QueueType::graphics).value();
	m_graphics_queue = device.get_queue(vkb::QueueType::graphics).value();

	init_allocator();
}

void VulkanEngine::init_allocator() {

	VmaAllocatorCreateInfo info = {};
	info.physicalDevice = m_physical_device;
	info.device = m_device;
	info.instance = m_instance;

	vmaCreateAllocator(&info, &m_allocator);
}

void VulkanEngine::init_depth_buffer() {

	m_depth_format = VK_FORMAT_D32_SFLOAT;

	VkExtent3D depth_extent = { m_windowExtent.width, m_windowExtent.height, 1 };

	VkImageCreateInfo info = vkinit::image_create_info(
		m_depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		depth_extent);

	VmaAllocationCreateInfo alloc_info = {};
	alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	alloc_info.requiredFlags =
		VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK(vmaCreateImage(m_allocator, &info, &alloc_info, &m_depth_image.image,
		&m_depth_image.allocation, nullptr));

	VkImageViewCreateInfo view_info = vkinit::imageview_create_info(
		m_depth_format, m_depth_image.image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(
		vkCreateImageView(m_device, &view_info, nullptr, &m_depth_image_view));

	m_main_deletion_queue.push_function([=]() {
		vkDestroyImageView(m_device, m_depth_image_view, nullptr);
		vmaFreeMemory(m_allocator, m_depth_image.allocation);
		vkDestroyImage(m_device, m_depth_image.image, nullptr);
		});
}

void VulkanEngine::init_swapchain() {
	ZoneScopedS(40);

	vkb::SwapchainBuilder builder{ m_physical_device, m_device, m_surface };
	vkb::Swapchain swapchain =
		builder.use_default_format_selection()
		.set_desired_extent(m_windowExtent.width, m_windowExtent.height)
		.set_desired_present_mode(VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR)
		.build()
		.value();

	m_swapchain = swapchain.swapchain;
	m_swapchain_image_format = swapchain.image_format;
	m_swapchain_images = swapchain.get_images().value();
	m_swapchain_image_views = swapchain.get_image_views().value();

	// init depth buffer
	init_depth_buffer();

	m_main_deletion_queue.push_function(
		[=]() { vkDestroySwapchainKHR(m_device, m_swapchain, nullptr); });
}

void VulkanEngine::init_commands() {

	VkCommandPoolCreateInfo cmd_info = vkinit::command_pool_create_info(
		m_graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (int i = 0; i < FRAME_OVERLAP; i++) {

		VK_CHECK(vkCreateCommandPool(m_device, &cmd_info, nullptr,
			&m_frames[i].command_pool));

		VkCommandBufferAllocateInfo cmd_alloc_info =
			vkinit::command_buffer_allocate_info(m_frames[i].command_pool, 1,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		VK_CHECK(vkAllocateCommandBuffers(m_device, &cmd_alloc_info,
			&m_frames[i].command_buffer));

		m_main_deletion_queue.push_function([=]() {
			vkDestroyCommandPool(m_device, m_frames[i].command_pool, nullptr);
			});
	}

	//create pool for upload context
	VkCommandPoolCreateInfo uploadCommandPoolInfo = vkinit::command_pool_create_info(m_graphics_queue_family);
	VK_CHECK(vkCreateCommandPool(m_device, &uploadCommandPoolInfo, nullptr, &m_gpuUploadConxtext.commandPool));

	m_main_deletion_queue.push_function([=]() {
		vkDestroyCommandPool(m_device, m_gpuUploadConxtext.commandPool, nullptr);
		});

}

void VulkanEngine::init_render_pass() {

	VkAttachmentDescription color_att = {};
	color_att.format = m_swapchain_image_format;
	color_att.samples = VK_SAMPLE_COUNT_1_BIT;
	color_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	color_att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_att_ref = {};
	color_att_ref.attachment = 0;
	color_att_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentDescription depth_attachment = {};

	// Depth attachment
	depth_attachment.flags = 0;
	depth_attachment.format = m_depth_format;
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout =
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_ref = {};
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout =
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_att_ref;
	subpass.pDepthStencilAttachment = &depth_attachment_ref;

	VkAttachmentDescription attachments[2];
	//= {, color_att};//

	attachments[0] = color_att;
	attachments[1] = depth_attachment;

	VkRenderPassCreateInfo render_pass_info = {
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	render_pass_info.attachmentCount = 2;
	render_pass_info.pAttachments = &attachments[0];
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	VK_CHECK(
		vkCreateRenderPass(m_device, &render_pass_info, nullptr, &m_render_pass));

	m_main_deletion_queue.push_function(
		[=]() { vkDestroyRenderPass(m_device, m_render_pass, nullptr); });
}

void VulkanEngine::init_framebuffers() {

	VkFramebufferCreateInfo fb_info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	fb_info.renderPass = m_render_pass;
	fb_info.width = m_windowExtent.width;
	fb_info.height = m_windowExtent.height;
	// fb_info.attachmentCount = 1;
	fb_info.layers = 1;

	const size_t swapchain_image_count = m_swapchain_images.size();

	m_framebuffers = std::vector<VkFramebuffer>(swapchain_image_count);

	for (uint32_t i = 0; i < swapchain_image_count; i++) {

		VkImageView attachments[2];
		attachments[0] = m_swapchain_image_views[i];
		attachments[1] = m_depth_image_view;
		fb_info.pAttachments = attachments;
		fb_info.attachmentCount = 2;

		VK_CHECK(
			vkCreateFramebuffer(m_device, &fb_info, nullptr, &m_framebuffers[i]));

		m_main_deletion_queue.push_function([=]() {
			vkDestroyFramebuffer(m_device, m_framebuffers[i], nullptr);

			vkDestroyImageView(m_device, m_swapchain_image_views[i], nullptr);
			});
	}
}

void VulkanEngine::init_sync_structs() {

	VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSemaphoreCreateInfo info_sem = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	for (int i = 0; i < FRAME_OVERLAP; i++) {

		VkFence& render_fence = m_frames[i].render_fence;
		VkSemaphore& render_semaphore = m_frames[i].render_semaphore;
		VkSemaphore& present_semaphore = m_frames[i].present_semaphore;

		VK_CHECK(vkCreateFence(m_device, &fence_info, nullptr, &render_fence));
		VK_CHECK(
			vkCreateSemaphore(m_device, &info_sem, nullptr, &render_semaphore));
		VK_CHECK(
			vkCreateSemaphore(m_device, &info_sem, nullptr, &present_semaphore));

		m_main_deletion_queue.push_function([=]() {
			vkDestroyFence(m_device, render_fence, nullptr);

			vkDestroySemaphore(m_device, present_semaphore, nullptr);

			vkDestroySemaphore(m_device, render_semaphore, nullptr);
			});
	}

	VkFenceCreateInfo gpuUplaodFenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	//gpuUplaodFenceCreateInfo.flags = 
	VK_CHECK(vkCreateFence(m_device, &gpuUplaodFenceCreateInfo, 0, &m_gpuUploadConxtext.uploadFence));
	m_main_deletion_queue.push_function([=]() {
		vkDestroyFence(m_device, m_gpuUploadConxtext.uploadFence, nullptr);
		});



}
void VulkanEngine::cleanup() {
	if (!m_isInitialized) {
		return;
	}

	for (int i = 0; i < FRAME_OVERLAP; i++) {

		VK_CHECK(vkWaitForFences(m_device, 1, &m_frames[i].render_fence, true,
			1000000000));
	}

	m_main_deletion_queue.flush();

	vmaDestroyAllocator(m_allocator);

	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyDevice(m_device, nullptr);
	vkb::destroy_debug_utils_messenger(m_instance, m_debug_messenger, nullptr);
	vkDestroyInstance(m_instance, nullptr);

	SDL_DestroyWindow(m_window);
}

void VulkanEngine::draw() {
	FrameMark;
	VK_CHECK(vkWaitForFences(m_device, 1, &get_current_frame().render_fence, true,
		1000000000));
	VK_CHECK(vkResetFences(m_device, 1, &get_current_frame().render_fence));


	uint32_t swapchain_image_index;
	VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX,
		get_current_frame().present_semaphore, nullptr,
		&swapchain_image_index));

	VK_CHECK(vkResetCommandBuffer(get_current_frame().command_buffer, 0));
	VkCommandBuffer cmd = get_current_frame().command_buffer;

	VkCommandBufferBeginInfo cmd_begin_info = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

	VkClearValue clear_val;
	const float flash = abs(sin(m_frameNumber / 120.f));
	clear_val.color = { {0.0f, 0.0f, flash, 1.0f} };

	// clear depth at 1
	VkClearValue depth_clear;
	depth_clear.depthStencil.depth = 1.0f;

	VkClearValue clearValues[] = { clear_val, depth_clear };

	VkRenderPassBeginInfo rp_info = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	rp_info.clearValueCount = 2;
	rp_info.pClearValues = &clearValues[0];
	rp_info.renderPass = m_render_pass;
	rp_info.framebuffer = m_framebuffers[swapchain_image_index];

	rp_info.renderArea.extent = m_windowExtent;
	rp_info.renderArea.offset = {
		0,
		0,
	};

	vkCmdBeginRenderPass(cmd, &rp_info, VK_SUBPASS_CONTENTS_INLINE);

	draw_objects(cmd, m_renderables.data(), m_renderables.size());

	vkCmdEndRenderPass(cmd);
	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submit_info.pWaitSemaphores = &get_current_frame().present_semaphore;
	submit_info.waitSemaphoreCount = 1;
	;

	submit_info.pSignalSemaphores = &get_current_frame().render_semaphore;
	submit_info.signalSemaphoreCount = 1;

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd;

	VkPipelineStageFlags wait_stage =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	submit_info.pWaitDstStageMask = &wait_stage;

	VK_CHECK(vkQueueSubmit(m_graphics_queue, 1, &submit_info,
		get_current_frame().render_fence));

	VkPresentInfoKHR present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &get_current_frame().render_semaphore;

	present_info.pImageIndices = &swapchain_image_index;

	present_info.swapchainCount = 1;
	present_info.pSwapchains = &m_swapchain;

	VK_CHECK(vkQueuePresentKHR(m_graphics_queue, &present_info));

	m_frameNumber++;
}

bool VulkanEngine::load_shader_module(const char* file_path,
	VkShaderModule* out) {
	std::ifstream file(file_path, std::ios_base::ate | std::ios::binary);
	if (!file.is_open()) {
		return false;
	}

	size_t file_size = (size_t)file.tellg();
	std::vector<uint32_t> buf(file_size / sizeof(uint32_t));
	file.seekg(0);

	file.read((char*)buf.data(), file_size);

	file.close();

	VkShaderModuleCreateInfo createInfo = {
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = file_size;
	createInfo.pCode = buf.data();

	VkShaderModule shader_module;

	if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shader_module) !=
		VK_SUCCESS) {
		return false;
	}

	*out = shader_module;

	return true;
}

void VulkanEngine::init_pipelines() {
	vkinit::PipelineBuilder pipeline_builder{};


	pipeline_builder.m_vertexInputInfo = vkinit::vertex_input_state_create_info();
	pipeline_builder.m_inputAssembly =
		vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	// viewport
	pipeline_builder.m_viewport.x = 0.0f;
	pipeline_builder.m_viewport.y = 0.0f;
	pipeline_builder.m_viewport.width = static_cast<float>(m_windowExtent.width);
	pipeline_builder.m_viewport.height = static_cast<float>(m_windowExtent.height);
	pipeline_builder.m_viewport.minDepth = 0.0f;
	pipeline_builder.m_viewport.maxDepth = 1.0f;

	pipeline_builder.m_scissor.offset = { 0, 0 };
	pipeline_builder.m_scissor.extent = m_windowExtent;

	pipeline_builder.m_rasterizer =
		vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);

	pipeline_builder.m_multisampling = vkinit::multisampling_state_create_info();

	pipeline_builder.m_colorBlendAttachment =
		vkinit::color_blend_attachment_state();


	pipeline_builder.m_depth_stencil = vkinit::depth_stencil_create_info(
		true, true, VK_COMPARE_OP_LESS_OR_EQUAL);


	init_triangle_mesh_pipeline(pipeline_builder);
}

void VulkanEngine::init_triangle_mesh_pipeline(
	vkinit::PipelineBuilder& builder) {
	VertexInputDescription vertex_description = Vertex::get_vertex_description();

	builder.m_vertexInputInfo.pVertexBindingDescriptions =
		vertex_description.bindings.data();
	builder.m_vertexInputInfo.vertexBindingDescriptionCount =
		vertex_description.bindings.size();

	builder.m_vertexInputInfo.pVertexAttributeDescriptions =
		vertex_description.attributes.data();
	builder.m_vertexInputInfo.vertexAttributeDescriptionCount =
		vertex_description.attributes.size();

	builder.m_shaderStages.clear();

	VkShaderModule vertex, frag;

	if (!load_shader_module("../../shaders/tri_mesh.vert.spv", &vertex)) {
		printf("error while trying to create a shader module !\n");
	}

	if (!load_shader_module("../../shaders/tri_mesh.frag.spv", &frag)) {
		printf("error while trying to create a shader module !\n");
	}

	VkPipelineShaderStageCreateInfo vertex_shader, fragment_Shader;

	vertex_shader = vkinit::pipeline_shader_stage_create_info(
		VK_SHADER_STAGE_VERTEX_BIT, vertex);
	fragment_Shader = vkinit::pipeline_shader_stage_create_info(
		VK_SHADER_STAGE_FRAGMENT_BIT, frag);

	builder.m_shaderStages.push_back(vertex_shader);
	builder.m_shaderStages.push_back(fragment_Shader);

	// mesh pipeline layout

	VkPipelineLayoutCreateInfo mesh_pipeline_layout_info =
		vkinit::pipeline_layout_create_info();

	VkPushConstantRange push_constant;
	push_constant.offset = 0;
	push_constant.size = sizeof(MeshPushConstants);
	push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	mesh_pipeline_layout_info.pPushConstantRanges = &push_constant;
	mesh_pipeline_layout_info.pushConstantRangeCount = 1;

	const VkDescriptorSetLayout setLayouts[] = { m_descriptorSetLayout , m_objectSetLayout, m_textureSetlayout };
	mesh_pipeline_layout_info.pSetLayouts = setLayouts;
	mesh_pipeline_layout_info.setLayoutCount = sizeof(setLayouts) / sizeof(setLayouts[0]);

	VK_CHECK(vkCreatePipelineLayout(m_device, &mesh_pipeline_layout_info, nullptr,
		&m_mesh_pipeline_layout));

	builder.m_pipelineLayout = m_mesh_pipeline_layout;

	//  builder.m_depth_stencil = vkinit::depth_stencil_create_info(
	//    true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

	m_mesh_pipeline = builder.build_pipeline(m_device, m_render_pass);

	create_material(m_mesh_pipeline, m_mesh_pipeline_layout, "texturedMesh");

	vkDestroyShaderModule(m_device, vertex, nullptr);
	vkDestroyShaderModule(m_device, frag, nullptr);

	m_main_deletion_queue.push_function([=]() {
		vkDestroyPipeline(m_device, m_mesh_pipeline, nullptr);

		//vkDestroyPipeline(m_device, m_triangle_pipeline, nullptr);

		vkDestroyPipelineLayout(m_device, m_mesh_pipeline_layout, nullptr);

		//vkDestroyPipelineLayout(m_device, m_triangle_pipeline_layout, nullptr);
		});
}
void VulkanEngine::init_scene() {

	//RenderObject monkey;
	//monkey.mesh = get_mesh("monkey");
	//monkey.material = get_material("basic_mesh");
	//monkey.transform = glm::mat4{ 1.0 };
	//m_renderables.push_back(monkey);


	////add random triangles 
	//for (int x = -20; x <= 20; x++) {
	//	for (int y = -20; y <= 20; y++) {

	//		RenderObject tri;
	//		tri.mesh = get_mesh("triangle");
	//		tri.material = get_material("basic_mesh");
	//		glm::mat4 translation =
	//			glm::translate(glm::mat4{ 1.0 }, glm::vec3(x, 0, y));
	//		glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.2, 0.2, 0.2));
	//		tri.transform = translation * scale;

	//		m_renderables.push_back(tri);
	//	}
	//}


	VkSamplerCreateInfo samplerInfo = vkinit::samplerCreateInfo(VK_FILTER_NEAREST);;
	VkSampler sampler;

	VK_CHECK(vkCreateSampler(m_device, &samplerInfo, 0, &sampler));

	Material* mat = get_material("texturedMesh");
	Texture& texture = m_textures["empire_diffuse"];
	VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = 1;;
	allocInfo.pSetLayouts = &m_textureSetlayout;

	VK_CHECK(vkAllocateDescriptorSets(m_device, &allocInfo, &mat->textureSet));

	//write to the set so that it points to our texture 
	VkDescriptorImageInfo imageBufferInfo = {};
	imageBufferInfo.sampler = sampler;
	imageBufferInfo.imageView = texture.imageView;
	imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet write = vkinit::writeDescriptorImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mat->textureSet, &imageBufferInfo, 0);

	vkUpdateDescriptorSets(m_device, 1, &write, 0, 0);


	RenderObject mesh = {};

	mesh.mesh = get_mesh("empire");
	mesh.material = get_material("texturedMesh");
	mesh.transform = glm::translate(glm::vec3(5, -10, 0));

	m_renderables.push_back(mesh);
}

void VulkanEngine::draw_objects(VkCommandBuffer cmd, RenderObject* first,
	size_t count) {



	glm::mat4 view = m_camera.getView(); // glm::translate(glm::mat4(1.f), camPos);
	// camera projection
	glm::mat4 projection =
		glm::perspective(glm::radians(100.f), 1700.f / 900.f, 0.1f, 200.0f);
	projection[1][1] *= -1;

	GPUCameraData cameraData;
	cameraData.projection = projection;
	cameraData.view = view;
	cameraData.viewProj = projection * view;

	//copy data to gpu 
	void* data;;
	vmaMapMemory(m_allocator, get_current_frame().cameraData.allocation, &data);
	memcpy(data, &cameraData, sizeof(GPUCameraData));
	vmaUnmapMemory(m_allocator, get_current_frame().cameraData.allocation);
	//
	// copy scene data to gpu buffer
	float framed = m_frameNumber / 120.0f;
	m_sceneData.ambientColor = { sin(framed),0,cos(framed),1 };

	char* sceneData;
	vmaMapMemory(m_allocator, m_sceneDataBuffer.allocation, (void**)&sceneData);

	int frameIndex = m_frameNumber % FRAME_OVERLAP;
	sceneData += alignUniformBufferSize(sizeof(GPUSceneData)) * frameIndex;

	memcpy(sceneData, &m_sceneData, sizeof(GPUSceneData));
	vmaUnmapMemory(m_allocator, m_sceneDataBuffer.allocation);


	FrameData& frameData = get_current_frame();
	// copy object data to gpu 
	void* objectData;
	vmaMapMemory(m_allocator, frameData.objectBuffer.allocation, &objectData);

	GPUObjectData* objectsBuffer = (GPUObjectData*)objectData;
	for (int i = 0; i < count; i++)
	{
		RenderObject& object = first[i];
		objectsBuffer[i].modelMatrix = object.transform;
	}

	vmaUnmapMemory(m_allocator, frameData.objectBuffer.allocation);
	//

	Mesh* lastMesh = nullptr;
	Material* lastMaterial = nullptr;
	for (int i = 0; i < count; i++) {
		RenderObject& object = first[i];


		// only bind the pipeline if it doesnt match with the already bound one
		if (object.material != lastMaterial) {

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				object.material->pipeline);
			lastMaterial = object.material;

			uint32_t uniformOffset = alignUniformBufferSize(sizeof(GPUSceneData)) * frameIndex;
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1, &frameData.descriptorSet, 1, &uniformOffset);

			//object data (transform matrix)
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 1, 1, &frameData.objectDescriptor, 0, 0);

			if (object.material->textureSet != VK_NULL_HANDLE) {
				//texture descriptor 
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 2, 1, &object.material->textureSet, 0, 0);
			}

		}

		//glm::mat4 mesh_matrix = object.transform;
		//MeshPushConstants constants;
		//constants.render_matrix = mesh_matrix;
		//// upload the mesh to the gpu via pushconstants
		//vkCmdPushConstants(cmd, object.material->pipelineLayout,
		//	VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants),
		//	&constants);


		// only bind the mesh if its a different one from last bind
		if (object.mesh != lastMesh) {
			// bind the mesh vertex buffer with offset 0
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->vertexBuffer.buffer,
				&offset);
			lastMesh = object.mesh;
		}
		// we can now draw
		vkCmdDraw(cmd, object.mesh->vertices.size(), 1, 0, i);
	}
}
void VulkanEngine::load_meshes() {
	m_triangle_mesh.vertices.resize(3);
	m_triangle_mesh.vertices[0].position = { 1.0f, -1.0f, 0.0f };
	m_triangle_mesh.vertices[1].position = { -1.0f, -1.0f, 0.0f };
	m_triangle_mesh.vertices[2].position = { 0.0f, 1.f, 0.0f };

	m_triangle_mesh.vertices[0].color = { 0.0f, 1.f, 0.0f };
	m_triangle_mesh.vertices[1].color = { 0.0f, 1.f, 0.0f };
	m_triangle_mesh.vertices[2].color = { 0.0f, 1.f, 0.0f };

	m_monkey.load_from_obj("../../assets/monkey_smooth.obj");

	upload_mesh(m_triangle_mesh);
	upload_mesh(m_monkey);

	m_meshes["monkey"] = m_monkey;
	m_meshes["triangle"] = m_triangle_mesh;

	//textured mesh
	Mesh mesh{};
	mesh.load_from_obj("../../assets/lost_empire.obj");
	upload_mesh(mesh);
	m_meshes["empire"] = mesh;

}

void VulkanEngine::upload_mesh(Mesh& mesh) {

	// creates a cpu buffer and a gpu buffer and copy the cpu buffer to the gpu buffer 

	VkBufferCreateInfo bufferInfo = {};

	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size =
		static_cast<VkDeviceSize>(mesh.vertices.size() * sizeof(Vertex));
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;


	AllocatedBuffer staggingBuffer;

	// allocate the gpu buffer
	VK_CHECK(vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo,
		&staggingBuffer.buffer,
		&staggingBuffer.allocation, nullptr));

	// copy to cpu buffer  
	void* data;
	vmaMapMemory(m_allocator, staggingBuffer.allocation, &data);
	memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
	vmaUnmapMemory(m_allocator, staggingBuffer.allocation);
	//copy cpu buffer to gpu buffer 
	{
		// prepare the gpu buffer 
		VkBufferCreateInfo bufferInfo = {};

		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size =
			static_cast<VkDeviceSize>(mesh.vertices.size() * sizeof(Vertex));
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		VK_CHECK(vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo,
			&mesh.vertexBuffer.buffer,
			&mesh.vertexBuffer.allocation, nullptr));

		immediateSubmit([=](VkCommandBuffer cmd) {
			VkBufferCopy cpy = {};
			cpy.size = static_cast<VkDeviceSize>(mesh.vertices.size() * sizeof(Vertex));
			(vkCmdCopyBuffer(cmd, staggingBuffer.buffer, mesh.vertexBuffer.buffer, 1, &cpy));

			});

		m_main_deletion_queue.push_function([=]() {
			vmaDestroyBuffer(m_allocator, mesh.vertexBuffer.buffer,
				mesh.vertexBuffer.allocation);
			});
		vmaDestroyBuffer(m_allocator, staggingBuffer.buffer, staggingBuffer.allocation);
	}
}

void VulkanEngine::run() {
	SDL_Event e;
	bool bQuit = false;
	const float amount = 1; 
	// main loop
	while (!bQuit) {
		// Handle events on queue
		while (SDL_PollEvent(&e) != 0) {
			// close the window when user alt-f4s or clicks the X button

			switch (e.type) {
			case SDL_QUIT:
				bQuit = true;
				break;
			case SDL_KEYDOWN: {
				switch (e.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					bQuit = true;
					break;
				case SDLK_w:
					m_camera.walk(amount);
					break;
				case SDLK_s:
					m_camera.walk(-amount);
					break;
				case SDLK_a:
					m_camera.strafe(-amount);
					break;
				case SDLK_d:
					m_camera.strafe(amount);
					break;
				default:
					break;
				}
				break;
			}
							//If mouse event happened
			case  SDL_MOUSEMOTION:
			case SDL_MOUSEBUTTONDOWN:
			case  SDL_MOUSEBUTTONUP:
			{
				//Get mouse position
				int x, y;
				SDL_GetMouseState(&x, &y);
		//		SDL_ShowCursor(0);
				SDL_SetRelativeMouseMode(SDL_TRUE);

				static int  oldX = 0, oldY = 0;

				static float dx = 0, dy = 0; 
				const float sensitivity = 0.005;;
				  dx += (x-oldX) *sensitivity;
				  dy += (y-oldY) * sensitivity;;

			
				  const float wrapX = 5* sensitivity;
				  if (x > m_windowExtent.width - 2) {
					  dx += wrapX; 
				  }
				  if (x <= 2) {
					  dx -= wrapX;
				  }

				  const float wrapY = 5* sensitivity;
				  if (y > m_windowExtent.height - 2) {
					  dy += wrapY;
				  }
				  if (y <= 2) {
					  dy -= wrapY;
				  }

				  const float pie = 3.14;
				  if (dy < -pie * 0.5) {
					  dy = -pie * 0.5;
				  }
				  if (dy > pie * 0.5) {
					  dy = pie * 0.5;
				  }


				oldX = x; 
				oldY = y; 
			
				m_camera.rotate(-dx,dy, 0);
				//m_camera.rotate(_x, glm::vec3{ 0.,1.0,0.0 });

				break;
			}

			}
		}
		m_camera.update();
		draw();
	}
}

Material* VulkanEngine::create_material(VkPipeline pipeline,
	VkPipelineLayout layout,
	const std::string& name) {
	Material mat;
	mat.pipeline = pipeline;
	mat.pipelineLayout = layout;
	m_materials[name] = mat;

	return &m_materials[name];
}

Material* VulkanEngine::get_material(const std::string& name) {

	auto it = m_materials.find(name);
	if (it == m_materials.end()) {
		return nullptr;
	}
	else {
		return &(*it).second;
	}
}

Mesh* VulkanEngine::get_mesh(const std::string& name) {

	auto it = m_meshes.find(name);

	if (it == m_meshes.end()) {
		return nullptr;
	}
	else {
		return &(*it).second;
	}
}

FrameData& VulkanEngine::get_current_frame() {

	return m_frames[m_frameNumber % FRAME_OVERLAP];
}


AllocatedBuffer VulkanEngine::createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {

	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = allocSize;
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo memInfo = {};
	memInfo.usage = memoryUsage;


	AllocatedBuffer buffer;


	VK_CHECK(vmaCreateBuffer(m_allocator, &bufferInfo, &memInfo, &buffer.buffer, &buffer.allocation, nullptr));

	m_main_deletion_queue.push_function([=]() {
		vmaDestroyBuffer(m_allocator, buffer.buffer,
			buffer.allocation);
		});

	return buffer;
}



void VulkanEngine::init_descriptors() {


	VkDescriptorSetLayoutBinding    cameraBind = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
	VkDescriptorSetLayoutBinding    sceneBind = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	VkDescriptorSetLayoutBinding bindings[] = { cameraBind,sceneBind };

	VkDescriptorSetLayoutBinding    objectBind = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
	VkDescriptorSetLayoutBinding    textureBind = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);



	VkDescriptorSetLayoutCreateInfo setInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	setInfo.pBindings = bindings;
	setInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
	VK_CHECK(vkCreateDescriptorSetLayout(m_device, &setInfo, 0, &m_descriptorSetLayout));

	VkDescriptorSetLayoutCreateInfo setInfo2 = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	setInfo2.pBindings = &objectBind;
	setInfo2.bindingCount = 1;
	VK_CHECK(vkCreateDescriptorSetLayout(m_device, &setInfo2, 0, &m_objectSetLayout));

	VkDescriptorSetLayoutCreateInfo setInfo3 = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	setInfo3.pBindings = &textureBind;
	setInfo3.bindingCount = 1;
	VK_CHECK(vkCreateDescriptorSetLayout(m_device, &setInfo3, 0, &m_textureSetlayout));


	std::vector<VkDescriptorPoolSize>  sizes = {
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,10} ,
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ,10 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,10 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,10 }
	};

	VkDescriptorPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };

	createInfo.pPoolSizes = sizes.data();
	createInfo.poolSizeCount = sizes.size();
	createInfo.maxSets = 10;

	VK_CHECK
	(vkCreateDescriptorPool(m_device, &createInfo, 0, &m_descriptorPool));


	const size_t sceneParamBufferSize = FRAME_OVERLAP * alignUniformBufferSize(sizeof(GPUSceneData));
	m_sceneDataBuffer = createBuffer(sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);


	for (int i = 0; i < FRAME_OVERLAP; i++) {
		m_frames[i].cameraData = createBuffer(sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		const int maxObjects = 10000;
		m_frames[i].objectBuffer = createBuffer(sizeof(GPUObjectData) * maxObjects, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = m_descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &m_descriptorSetLayout;

		//alloc the global descriptor set for each frame 
		VK_CHECK(vkAllocateDescriptorSets(m_device, &allocInfo, &m_frames[i].descriptorSet));

		// point to the wanted vkBuffer 

		VkDescriptorBufferInfo    cameraInfo = { };
		cameraInfo.buffer = m_frames[i].cameraData.buffer;
		cameraInfo.offset = 0;
		cameraInfo.range = sizeof(GPUCameraData);

		VkDescriptorBufferInfo    sceneInfo = { };
		sceneInfo.buffer = m_sceneDataBuffer.buffer;
		sceneInfo.offset = 0;
		sceneInfo.range = sizeof(GPUSceneData);

		VkWriteDescriptorSet cameraWrite = vkinit::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_frames[i].descriptorSet, &cameraInfo, 0);
		VkWriteDescriptorSet sceneWrite = vkinit::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, m_frames[i].descriptorSet, &sceneInfo, 1);

		VkWriteDescriptorSet setWrites[] = { cameraWrite, sceneWrite };

		vkUpdateDescriptorSets(m_device, sizeof(setWrites) / sizeof(setWrites[0]), setWrites, 0, 0);

		// alloc object descriptor set 

		VkDescriptorSetAllocateInfo objectAllocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		objectAllocInfo.descriptorPool = m_descriptorPool;
		objectAllocInfo.descriptorSetCount = 1;
		objectAllocInfo.pSetLayouts = &m_objectSetLayout;

		VK_CHECK(vkAllocateDescriptorSets(m_device, &objectAllocInfo, &m_frames[i].objectDescriptor));

		VkDescriptorBufferInfo   objectBufferInfo = { };
		objectBufferInfo.buffer = m_frames[i].objectBuffer.buffer;
		objectBufferInfo.offset = 0;
		objectBufferInfo.range = sizeof(GPUObjectData) * maxObjects
			;

		VkWriteDescriptorSet objectWrite = vkinit::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_frames[i].objectDescriptor, &objectBufferInfo, 0);
		vkUpdateDescriptorSets(m_device, 1, &objectWrite, 0, 0);
	}

}

size_t VulkanEngine::alignUniformBufferSize(size_t originalSize) {
	// Calculate required alignment based on minimum device offset alignment
	size_t minUboAlignment = m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
	size_t alignedSize = originalSize;
	if (minUboAlignment > 0) {
		alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}
	return alignedSize;
}


void VulkanEngine::load_images() {

	Texture tx;

	TextureManager::loadImageFromFile(*this, "../../assets/lost_empire-RGBA.png", tx.image);

	VkImageViewCreateInfo imageInfo = vkinit::imageview_create_info(VK_FORMAT_R8G8B8A8_SRGB, tx.image.image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(m_device, &imageInfo, 0, &tx.imageView));

	m_textures["empire_diffuse"] = tx;

}

