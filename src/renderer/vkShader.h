#pragma once
#include <vector>
#include "vulkan/vulkan.h"
namespace Sura {

struct ShaderModule {
  std::vector<unsigned int> spriv;
  VkShaderModule shaderModule;
};

class Shader {
  static void compileShaderFile(const char* src, ShaderModule& mod);
};

}  // namespace Sura
