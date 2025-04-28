#pragma once
#include <vulkan/vulkan.h>
#include <shaderc/shaderc.h>

enum class MyEnumClass
{

};

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* devMem);

VkShaderModule compileShader(VkDevice device, VkAllocationCallbacks* callbacks,
							 const char* path, const char* entryName,
							 shaderc_shader_kind shaderKind ,shaderc_compiler_t shaderCompiler);