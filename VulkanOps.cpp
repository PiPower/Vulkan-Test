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

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
					VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* devMem)
{
	//vkCreateBuffer()
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


