#pragma once

#include <vk_types.h>

class VulkanEngine;

struct Texture {
	AllocatedImage image;
	VkImageView imageView;
};



class TextureManager {
public:
	bool  static loadImageFromFile(VulkanEngine& engine, const char* file, AllocatedImage& outImage);
};

