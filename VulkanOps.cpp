#include "VulkanOps.hpp"
#include "VulkanBase.hpp"
#include <vector>
#include <string>
#include <fstream>

static std::vector<char> readFile(const std::string& filename)
{
    using namespace std;
    ifstream file(filename, ios::ate | ios::binary);

    if (!file.is_open())
    {
        MessageBox(NULL, L"Failed to open file", NULL, MB_OK);
        exit(-1);
    }

    size_t fileSize = (size_t)file.tellg();
    vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;

}

VkDeviceMemory allocateBuffer(VkDevice device,VkPhysicalDevice physicalDevice, 
                              VkMemoryRequirements memRequirements, VkMemoryPropertyFlags properties)
{
    VkDeviceMemory devMem;
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    uint32_t i;
    for (i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            break;
        }
    }

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = i;

    CHECK_VK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &devMem));
    return devMem;
}

Texture create2DTexture(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format)
{
    Texture texture = {};
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    CHECK_VK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &texture.texImage));

    VkMemoryRequirements memRequirements = {};
    vkGetImageMemoryRequirements(device, texture.texImage, &memRequirements);
    
    texture.alignment = memRequirements.alignment;
    texture.memory = allocateBuffer(device, physicalDevice, memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    return texture;
}

VkShaderModule compileShader(VkDevice device, VkAllocationCallbacks* callbacks,
                             const char* path, const char* entryName,
                             shaderc_shader_kind shaderKind, shaderc_compiler_t shaderCompiler)
{
	std::vector<char> shaderSrc = readFile(path);
    size_t fileNameOffset = strlen(path) - 1;
    while ( path[fileNameOffset] != '\\'  && path[fileNameOffset] != '/'  && fileNameOffset != 0)
    {
        fileNameOffset--;
    }

	shaderc_compilation_result_t result = shaderc_compile_into_spv(
		shaderCompiler, shaderSrc.data(), shaderSrc.size(),
        shaderKind, path + fileNameOffset, entryName, nullptr);

    shaderc_compilation_status errCount = shaderc_result_get_compilation_status(result);
    if (errCount != shaderc_compilation_status_success)
    {
        const char* errMsg  = shaderc_result_get_error_message(result);
        MessageBoxA(NULL, errMsg, NULL, MB_OK);
        exit(-1);
    }

    VkShaderModule compiledModule;
    VkShaderModuleCreateInfo moduleInfo = {};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = shaderc_result_get_length(result);
    moduleInfo.pCode = (uint32_t*)shaderc_result_get_bytes(result);
    CHECK_VK_RESULT(vkCreateShaderModule(device, &moduleInfo, callbacks, &compiledModule));

    shaderc_result_release(result);
    return compiledModule;
}


