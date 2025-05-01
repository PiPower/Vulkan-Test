#pragma once
#include <vulkan/vulkan.h>
#include <shaderc/shaderc.h>

enum class MyEnumClass
{

};

VkDeviceMemory allocateBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
							  VkMemoryRequirements memRequirements, VkMemoryPropertyFlags properties);

VkShaderModule compileShader(VkDevice device, VkAllocationCallbacks* callbacks,
							 const char* path, const char* entryName,
							 shaderc_shader_kind shaderKind ,shaderc_compiler_t shaderCompiler);