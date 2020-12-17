#include "vk_engine.h"
#pragma warning(disable : 26812)
#include <SDL.h>
#include <SDL_vulkan.h>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <vk_initializers.h>
#include <vk_types.h>
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "Tracy.hpp"
#include "common/TracySystem.hpp"

#define VK_CHECK(x)                                                            \
  do {                                                                         \
    VkResult err = x;                                                          \
    if (err) {                                                                 \
      std::cout << "Detected Vulkan error: " << err << std::endl;              \
      abort();                                                                 \
    }                                                                          \
  } while (0)

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
  init_pipelines();

  load_meshes();

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

  vkb::PhysicalDeviceSelector selector{vkb_inst};

  //
  auto physicalDeviceResult =
      selector.set_minimum_version(1, 1).set_surface(m_surface).select();
  vkb::PhysicalDevice physicalDevice;
  if (physicalDeviceResult.has_value()) {
    physicalDevice = physicalDeviceResult.value();
    printf("Using vulkan : 1.1\n");
  } else {
    auto physicalDeviceResult =
        selector.set_minimum_version(1, 0).set_surface(m_surface).select();
    physicalDevice = physicalDeviceResult.value();
    printf("Using vulkan : 1.0\n");
  }

  m_physical_device = physicalDevice.physical_device;

  vkb::DeviceBuilder deviceBuilder{physicalDevice};
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

  VkExtent3D depth_extent = {m_windowExtent.width, m_windowExtent.height, 1};

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

  vkb::SwapchainBuilder builder{m_physical_device, m_device, m_surface};
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
      VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
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

  VkFramebufferCreateInfo fb_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
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

  VkFenceCreateInfo fence_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  VkSemaphoreCreateInfo info_sem = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  for (int i = 0; i < FRAME_OVERLAP; i++) {

    VkFence &render_fence = m_frames[i].render_fence;
    VkSemaphore &render_semaphore = m_frames[i].render_semaphore;
    VkSemaphore &present_semaphore = m_frames[i].present_semaphore;

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
  VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain, 0,
                                 get_current_frame().present_semaphore, nullptr,
                                 &swapchain_image_index));

  VK_CHECK(vkResetCommandBuffer(get_current_frame().command_buffer, 0));
  VkCommandBuffer cmd = get_current_frame().command_buffer;

  VkCommandBufferBeginInfo cmd_begin_info = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

  VkClearValue clear_val;
  const float flash = abs(sin(m_frameNumber / 120.f));
  clear_val.color = {{0.0f, 0.0f, flash, 1.0f}};

  // clear depth at 1
  VkClearValue depth_clear;
  depth_clear.depthStencil.depth = 1.0f;

  VkClearValue clearValues[] = {clear_val, depth_clear};

  VkRenderPassBeginInfo rp_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
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
  // vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_mesh_pipeline);

  // VkDeviceSize offset = 0;
  //// push constant
  // glm::vec3 camPos = {0.f, 0.f, -2.f};

  // glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
  //// camera projection
  // glm::mat4 projection =
  //    glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 10000.0f);
  // projection[1][1] *= -1;
  //// model rotation
  // glm::mat4 model = glm::rotate(
  //    glm::mat4{1.0f}, glm::radians(m_frameNumber * 0.4f), glm::vec3(0, 1,
  //    0));

  //// calculate final mesh matrix
  // glm::mat4 mesh_matrix = projection * view * model;

  // MeshPushConstants constants;
  // constants.render_matrix = mesh_matrix;

  // vkCmdPushConstants(cmd, m_mesh_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT,
  // 0,
  //                   sizeof(MeshPushConstants), &constants);

  //// draw triangle
  // vkCmdBindVertexBuffers(cmd, 0, 1, &m_triangle_mesh.vertexBuffer.buffer,
  //                       &offset);

  // vkCmdDraw(cmd, m_triangle_mesh.vertices.size(), 1, 0, 0);

  //// draw moneky
  // vkCmdBindVertexBuffers(cmd, 0, 1, &m_monkey.vertexBuffer.buffer, &offset);

  // vkCmdDraw(cmd, m_monkey.vertices.size(), 1, 0, 0);

  vkCmdEndRenderPass(cmd);
  VK_CHECK(vkEndCommandBuffer(cmd));

  VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
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

  VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &get_current_frame().render_semaphore;

  present_info.pImageIndices = &swapchain_image_index;

  present_info.swapchainCount = 1;
  present_info.pSwapchains = &m_swapchain;

  VK_CHECK(vkQueuePresentKHR(m_graphics_queue, &present_info));

  m_frameNumber++;
}

bool VulkanEngine::load_shader_module(const char *file_path,
                                      VkShaderModule *out) {
  std::ifstream file(file_path, std::ios_base::ate | std::ios::binary);
  if (!file.is_open()) {
    return false;
  }

  size_t file_size = (size_t)file.tellg();
  std::vector<uint32_t> buf(file_size / sizeof(uint32_t));
  file.seekg(0);

  file.read((char *)buf.data(), file_size);

  file.close();

  VkShaderModuleCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
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
  VkShaderModule vertex_shader, frag_Shader;

  if (!load_shader_module("../../shaders/shader.vert.spv", &vertex_shader)) {
    printf("error while trying to create a shader module !\n");
  }

  if (!load_shader_module("../../shaders/shader.frag.spv", &frag_Shader)) {
    printf("error while trying to create a shader module !\n");
  }

  VkPipelineLayoutCreateInfo pipeline_layout_create_info =
      vkinit::pipeline_layout_create_info();
  VK_CHECK(vkCreatePipelineLayout(m_device, &pipeline_layout_create_info,
                                  nullptr, &m_triangle_pipeline_layout));

  vkinit::PipelineBuilder pipeline_builder{};

  pipeline_builder.m_shaderStages.push_back(
      vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT,
                                                vertex_shader));
  pipeline_builder.m_shaderStages.push_back(
      vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT,
                                                frag_Shader));

  pipeline_builder.m_vertexInputInfo = vkinit::vertex_input_state_create_info();
  pipeline_builder.m_inputAssembly =
      vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

  // viewport
  pipeline_builder.m_viewport.x = 0.0f;
  pipeline_builder.m_viewport.y = 0.0f;
  pipeline_builder.m_viewport.width = static_cast<float>(m_windowExtent.width);
  pipeline_builder.m_viewport.height =
      static_cast<float>(m_windowExtent.height);
  pipeline_builder.m_viewport.minDepth = 0.0f;
  pipeline_builder.m_viewport.maxDepth = 1.0f;

  pipeline_builder.m_scissor.offset = {0, 0};
  pipeline_builder.m_scissor.extent = m_windowExtent;

  pipeline_builder.m_rasterizer =
      vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);

  pipeline_builder.m_multisampling = vkinit::multisampling_state_create_info();

  pipeline_builder.m_colorBlendAttachment =
      vkinit::color_blend_attachment_state();

  pipeline_builder.m_pipelineLayout = m_triangle_pipeline_layout;

  pipeline_builder.m_depth_stencil = vkinit::depth_stencil_create_info(
      true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

  m_triangle_pipeline =
      pipeline_builder.build_pipeline(m_device, m_render_pass);

  vkDestroyShaderModule(m_device, frag_Shader, nullptr);
  vkDestroyShaderModule(m_device, vertex_shader, nullptr);

  init_triangle_mesh_pipeline(pipeline_builder);
}

void VulkanEngine::init_triangle_mesh_pipeline(
    vkinit::PipelineBuilder &builder) {
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
  VK_CHECK(vkCreatePipelineLayout(m_device, &mesh_pipeline_layout_info, nullptr,
                                  &m_mesh_pipeline_layout));

  builder.m_pipelineLayout = m_mesh_pipeline_layout;

  //  builder.m_depth_stencil = vkinit::depth_stencil_create_info(
  //    true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

  m_mesh_pipeline = builder.build_pipeline(m_device, m_render_pass);

  create_material(m_mesh_pipeline, m_mesh_pipeline_layout, "basic_mesh");

  vkDestroyShaderModule(m_device, vertex, nullptr);
  vkDestroyShaderModule(m_device, frag, nullptr);

  m_main_deletion_queue.push_function([=]() {
    vkDestroyPipeline(m_device, m_mesh_pipeline, nullptr);

    vkDestroyPipeline(m_device, m_triangle_pipeline, nullptr);

    vkDestroyPipelineLayout(m_device, m_mesh_pipeline_layout, nullptr);

    vkDestroyPipelineLayout(m_device, m_triangle_pipeline_layout, nullptr);
  });
}
void VulkanEngine::init_scene() {

  RenderObject monkey;
  monkey.mesh = get_mesh("monkey");
  monkey.material = get_material("basic_mesh");
  monkey.transform = glm::mat4{1.0};
  m_renderables.push_back(monkey);

  for (int x = -20; x <= 20; x++) {
    for (int y = -20; y <= 20; y++) {

      RenderObject tri;
      tri.mesh = get_mesh("triangle");
      tri.material = get_material("basic_mesh");
      glm::mat4 translation =
          glm::translate(glm::mat4{1.0}, glm::vec3(x, 0, y));
      glm::mat4 scale = glm::scale(glm::mat4{1.0}, glm::vec3(0.2, 0.2, 0.2));
      tri.transform = translation * scale;

      m_renderables.push_back(tri);
    }
  }
}
void VulkanEngine::draw_objects(VkCommandBuffer cmd, RenderObject *first,
                                size_t count) {
  glm::vec3 camPos = {0.f, -6.f, -10.f};

  glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
  // camera projection
  glm::mat4 projection =
      glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
  projection[1][1] *= -1;

  Mesh *lastMesh = nullptr;
  Material *lastMaterial = nullptr;
  for (int i = 0; i < count; i++) {
    RenderObject &object = first[i];

    // only bind the pipeline if it doesnt match with the already bound one
    if (object.material != lastMaterial) {

      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        object.material->pipeline);
      lastMaterial = object.material;
    }

    glm::mat4 model = object.transform;
    // final render matrix, that we are calculating on the cpu
    glm::mat4 mesh_matrix = projection * view * model;

    MeshPushConstants constants;
    constants.render_matrix = mesh_matrix;

    // upload the mesh to the gpu via pushconstants
    vkCmdPushConstants(cmd, object.material->pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants),
                       &constants);

    // only bind the mesh if its a different one from last bind
    if (object.mesh != lastMesh) {
      // bind the mesh vertex buffer with offset 0
      VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->vertexBuffer.buffer,
                             &offset);
      lastMesh = object.mesh;
    }
    // we can now draw
    vkCmdDraw(cmd, object.mesh->vertices.size(), 1, 0, 0);
  }
}
void VulkanEngine::load_meshes() {
  m_triangle_mesh.vertices.resize(3);
  m_triangle_mesh.vertices[0].position = {1.0f, -1.0f, 0.0f};
  m_triangle_mesh.vertices[1].position = {-1.0f, -1.0f, 0.0f};
  m_triangle_mesh.vertices[2].position = {0.0f, 1.f, 0.0f};

  m_triangle_mesh.vertices[0].color = {0.0f, 1.f, 0.0f};
  m_triangle_mesh.vertices[1].color = {0.0f, 1.f, 0.0f};
  m_triangle_mesh.vertices[2].color = {0.0f, 1.f, 0.0f};

  m_monkey.load_from_obj("../../assets/monkey_smooth.obj");

  upload_mesh(m_triangle_mesh);
  upload_mesh(m_monkey);

  m_meshes["monkey"] = m_monkey;
  m_meshes["triangle"] = m_triangle_mesh;
}

void VulkanEngine::upload_mesh(Mesh &mesh) {
  VkBufferCreateInfo bufferInfo = {};

  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size =
      static_cast<VkDeviceSize>(mesh.vertices.size() * sizeof(Vertex));
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

  // allocate the gpu buffer
  VK_CHECK(vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo,
                           &mesh.vertexBuffer.buffer,
                           &mesh.vertexBuffer.allocation, nullptr));

  // copy from cpu to gpu
  void *data;
  vmaMapMemory(m_allocator, mesh.vertexBuffer.allocation, &data);
  memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
  vmaUnmapMemory(m_allocator, mesh.vertexBuffer.allocation);

  m_main_deletion_queue.push_function([=]() {
    vmaDestroyBuffer(m_allocator, mesh.vertexBuffer.buffer,
                     mesh.vertexBuffer.allocation);
  });
}

void VulkanEngine::run() {
  SDL_Event e;
  bool bQuit = false;

  // main loop
  while (!bQuit) {
    // Handle events on queue
    while (SDL_PollEvent(&e) != 0) {
      // close the window when user alt-f4s or clicks the X button

      switch (e.type) {
      case SDL_QUIT:
        bQuit = true;
        break;
      case SDL_KEYDOWN:
        if (e.key.keysym.sym == SDLK_ESCAPE) {
          bQuit = true;
        }
        break;
      default:
        break;
      }
      if (e.type == SDL_QUIT)
        bQuit = true;
    }

    draw();
  }
}

Material *VulkanEngine::create_material(VkPipeline pipeline,
                                        VkPipelineLayout layout,
                                        const std::string &name) {
  Material mat;
  mat.pipeline = pipeline;
  mat.pipelineLayout = layout;
  m_materials[name] = mat;

  return &m_materials[name];
}

Material *VulkanEngine::get_material(const std::string &name) {

  auto it = m_materials.find(name);
  if (it == m_materials.end()) {
    return nullptr;
  } else {
    return &(*it).second;
  }
}

Mesh *VulkanEngine::get_mesh(const std::string &name) {

  auto it = m_meshes.find(name);

  if (it == m_meshes.end()) {
    return nullptr;
  } else {
    return &(*it).second;
  }
}

FrameData &VulkanEngine::get_current_frame() {

  return m_frames[m_frameNumber % FRAME_OVERLAP];
}
