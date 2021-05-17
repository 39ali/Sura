#pragma once
#include "vk_mesh.h"
#include "vk_types.h"
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
struct Material {
  VkPipeline pipeline;
  VkPipelineLayout pipelineLayout;
  VkDescriptorSet textureSet ;
};

struct RenderObject {
  Mesh *mesh;
  Material *material;
  glm::mat4 transform;
};
