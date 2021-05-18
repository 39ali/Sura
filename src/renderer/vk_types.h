
#pragma once
#include <glm/glm.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

struct AllocatedBuffer {
	VkBuffer buffer;
	VmaAllocation allocation;
};

struct MeshPushConstants {
	glm::vec4 data;
	glm::mat4 render_matrix;
};

struct AllocatedImage {
	VkImage image;
	VmaAllocation allocation;
};


#define VK_CHECK(x)                                                            \
  do {                                                                         \
    VkResult err = x;                                                          \
    if (err) {                                                                 \
      printf("Detected Vulkan error: %d \n" , err);							   \
      abort();                                                                 \
    }                                                                          \
  } while (0)


