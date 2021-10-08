#include "texture.h"
#include "vk_initializers.h"
#include "vk_engine.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>



bool TextureManager::loadImageFromFile(VulkanEngine& engine, const char* file, AllocatedImage& outImage) {

	int width, height, channels;

	stbi_uc* pixels = stbi_load(file, &width, &height, &channels, STBI_rgb_alpha);

	if (!pixels) {
		assert(!"image loading error!");
	}

	void* pixelPtr = pixels;

	VkDeviceSize imageSize = static_cast<VkDeviceSize> (width * height * 4);



	AllocatedBuffer stagingBuffer = engine.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	//copy image to buffer 
	void* data;
	vmaMapMemory(engine.m_allocator, stagingBuffer.allocation, &data);
	memcpy(data, pixelPtr, imageSize);
	vmaUnmapMemory(engine.m_allocator, stagingBuffer.allocation);

	stbi_image_free(pixels);

	VkFormat format =
		VK_FORMAT_R8G8B8A8_SRGB;

	VkExtent3D imageExtent;
	imageExtent.width = static_cast<uint32_t>(width);
	imageExtent.height= static_cast<uint32_t>(height);
	imageExtent.depth = 1;

	AllocatedImage img;
	VkImageCreateInfo imgInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);

	VmaAllocationCreateInfo imgAlloc = {};
	imgAlloc.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VK_CHECK(vmaCreateImage(engine.m_allocator, &imgInfo, &imgAlloc, &img.image, &img.allocation, nullptr));

	engine.immediateSubmit([&](VkCommandBuffer cmd) {

		VkImageSubresourceRange range = {};
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.levelCount = 1;
		range.layerCount = 1;;

		VkImageMemoryBarrier toTransfer = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		toTransfer.image = img.image;
		toTransfer.subresourceRange = range;

		toTransfer.srcAccessMask = 0;
		toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1, &toTransfer);

		VkBufferImageCopy copy = {};
		copy.imageExtent = imageExtent;
		copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy.imageSubresource.layerCount = 1;
		copy.imageSubresource.baseArrayLayer = 0;
		copy.imageSubresource.mipLevel = 0;


		vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer
			, img.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

		// transfer image to a layout readable by the shader 
		VkImageMemoryBarrier toReadable = toTransfer;
		toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, 0, 0, 0, 1, &toReadable);

		});

	engine.m_main_deletion_queue.push_function([=]() {
		vmaDestroyImage(engine.m_allocator, img.image, img.allocation);
		});


	outImage = img;

	printf("texture loaded succesfully:%s \n", file);

	return true;
}
