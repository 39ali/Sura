#pragma once
#pragma warning(disable : 26812)
#include <vector>
#include "vk_types.h"
namespace vkinit {

class PipelineBuilder {
public:
  std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
  VkPipelineVertexInputStateCreateInfo m_vertexInputInfo;
  VkPipelineInputAssemblyStateCreateInfo m_inputAssembly;
  VkViewport m_viewport;
  VkRect2D m_scissor;
  VkPipelineRasterizationStateCreateInfo m_rasterizer;
  VkPipelineColorBlendAttachmentState m_colorBlendAttachment;
  VkPipelineMultisampleStateCreateInfo m_multisampling;
  VkPipelineLayout m_pipelineLayout;
  VkPipelineDepthStencilStateCreateInfo m_depth_stencil;

  VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);
  void rest() {
    m_shaderStages = {};
    m_vertexInputInfo = {};
    m_inputAssembly = {};
    m_viewport = {};
    m_scissor = {};
    m_rasterizer = {};
    m_colorBlendAttachment = {};
    m_multisampling = {};
    m_pipelineLayout = {};
  }
};

VkCommandPoolCreateInfo
command_pool_create_info(uint32_t queueFamilyIndex,
                         VkCommandPoolCreateFlags flags = 0);

VkCommandBufferAllocateInfo command_buffer_allocate_info(
    VkCommandPool pool, uint32_t count = 1,
    VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

VkPipelineShaderStageCreateInfo
pipeline_shader_stage_create_info(VkShaderStageFlagBits stage,
                                  VkShaderModule shader_module);

VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();

VkPipelineInputAssemblyStateCreateInfo
input_assembly_create_info(VkPrimitiveTopology topology);

VkPipelineRasterizationStateCreateInfo
rasterization_state_create_info(VkPolygonMode polygonMode);

VkPipelineMultisampleStateCreateInfo multisampling_state_create_info();

VkPipelineColorBlendAttachmentState color_blend_attachment_state();

VkPipelineLayoutCreateInfo pipeline_layout_create_info();

VkImageCreateInfo image_create_info(VkFormat format,
                                    VkImageUsageFlags usageFlags,
                                    VkExtent3D extent);

VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image,
                                            VkImageAspectFlags aspectFlags);

VkPipelineDepthStencilStateCreateInfo
depth_stencil_create_info(bool depth_test, bool depth_write,
                          VkCompareOp compareOp);

VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
VkWriteDescriptorSet writeDescriptorBuffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding);

VkSamplerCreateInfo samplerCreateInfo(VkFilter filters , VkSamplerAddressMode samplerAdressMode= VK_SAMPLER_ADDRESS_MODE_REPEAT );
VkWriteDescriptorSet writeDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding);

} // namespace vkinit
