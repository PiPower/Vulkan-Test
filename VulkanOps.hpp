#pragma once
#include <vulkan/vulkan.h>
#include <shaderc/shaderc.h>
#include <vector>

struct Texture
{
	VkImage texImage;
	VkDeviceMemory memory;
	VkImageView texView;
	VkDeviceSize alignment;
};

struct UniformBuffer
{
	VkBuffer globalBuffer;
	std::vector<VkBuffer> subBuffers;
	VkDeviceMemory buffMem;
};

struct BufferMemoryProperties
{
	VkDeviceSize alignment; 
	VkDeviceSize size;
};
VkDeviceMemory allocateBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
							  VkMemoryRequirements memRequirements, VkMemoryPropertyFlags properties);
Texture create2DTexture(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format);

VkShaderModule compileShader(VkDevice device, VkAllocationCallbacks* callbacks,
							 const char* path, const char* entryName,
							 shaderc_shader_kind shaderKind ,shaderc_compiler_t shaderCompiler);
UniformBuffer createUniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize buffSize,
							std::vector<VkDeviceSize> offsets = {}, std::vector<VkDeviceSize> chunkLenghts = {});
BufferMemoryProperties getBufferMemoryProperties(VkDevice device, VkDeviceSize buffSize, VkBufferUsageFlags usage);
