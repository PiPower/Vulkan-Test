#pragma once
#include <vulkan/vulkan.h>
#include <shaderc/shaderc.h>

struct Texture
{
	VkImage texImage;
	VkDeviceMemory memory;
	VkImageView texView;
	VkDeviceSize alignment;
};

VkDeviceMemory allocateBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
							  VkMemoryRequirements memRequirements, VkMemoryPropertyFlags properties);
Texture create2DTexture(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format);

VkShaderModule compileShader(VkDevice device, VkAllocationCallbacks* callbacks,
							 const char* path, const char* entryName,
							 shaderc_shader_kind shaderKind ,shaderc_compiler_t shaderCompiler);