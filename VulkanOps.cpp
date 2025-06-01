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
    vector<char> buffer(fileSize + 1);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    buffer[fileSize] = '\0';
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

    CHECK_VK_RESULT(vkBindImageMemory(device, texture.texImage, texture.memory, 0));

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = texture.texImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    CHECK_VK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &texture.texView));
    return texture;
}

VkShaderModule compileShader(VkDevice device, VkAllocationCallbacks* callbacks,
                             const char* path, const char* entryName,
                             shaderc_shader_kind shaderKind, shaderc_compiler_t shaderCompiler, 
                             shaderc_compile_options_t options)
{
	std::vector<char> shaderSrc = readFile(path);
    size_t fileNameOffset = strlen(path) - 1;

    while ( path[fileNameOffset] != '\\'  && path[fileNameOffset] != '/'  && fileNameOffset != 0)
    {
        fileNameOffset--;
    }

	shaderc_compilation_result_t result = shaderc_compile_into_spv(
		shaderCompiler, shaderSrc.data(), shaderSrc.size() - 1,
        shaderKind, path + fileNameOffset, entryName, options);

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

UniformBuffer createUniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize buffSize, 
                                    std::vector<VkDeviceSize> offsets, std::vector<VkDeviceSize> chunkLenghts)
{
    if (offsets.size() != chunkLenghts.size())
    {
        OutputDebugString(L"Error: offsets.size() != chunkLenghts.size()\n");
        exit(-1);
    }

    UniformBuffer out = {};
    VkBufferCreateInfo buffInfo = {};
    buffInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffInfo.size = buffSize;
    buffInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    CHECK_VK_RESULT( vkCreateBuffer(device, &buffInfo, nullptr, &out.globalBuffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, out.globalBuffer, &memRequirements);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    uint32_t i;
    VkMemoryPropertyFlags props = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    for (i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & props) == props) {
            break;
        }
    }

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = i;

    CHECK_VK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &out.buffMem));
    CHECK_VK_RESULT(vkBindBufferMemory(device, out.globalBuffer, out.buffMem, 0));

    for (size_t i = 0; i < offsets.size(); i++)
    {
        out.subBuffers.push_back(VK_NULL_HANDLE);

        VkBufferCreateInfo buffInfo = {};
        buffInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffInfo.size = chunkLenghts[i];
        buffInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buffInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        CHECK_VK_RESULT(vkCreateBuffer(device, &buffInfo, nullptr, &out.subBuffers[i]));
        CHECK_VK_RESULT(vkBindBufferMemory(device, out.subBuffers[i], out.buffMem, offsets[i]));
    }

    return out;
}

BufferMemoryProperties getBufferMemoryProperties(VkDevice device, VkDeviceSize buffSize, VkBufferUsageFlags usage)
{
    BufferMemoryProperties out = {};

    VkBuffer tmpBuffer;
    VkBufferCreateInfo buffInfo = {};
    buffInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffInfo.size = buffSize;
    buffInfo.usage = usage;
    buffInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(device, &buffInfo, nullptr, &tmpBuffer);

    VkMemoryRequirements memReqs = {};
    vkGetBufferMemoryRequirements(device, tmpBuffer, &memReqs);
    out.alignment = memReqs.alignment;
    out.size = memReqs.size;

    vkDestroyBuffer(device, tmpBuffer, nullptr);
    return out;
}


