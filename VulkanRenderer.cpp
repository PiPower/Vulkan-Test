#include "VulkanRenderer.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
using namespace std;
struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 2.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};


struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    //matrix proj;
};


VulkanRenderer::VulkanRenderer(HINSTANCE hinstance, HWND hwnd)
{
    vulkanBase = createVulkanBase(hinstance, hwnd);
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateUbo();
    CreatePipelineLayout();
    CreatePoolAndSets();
    CreateGraphicsPipeline();
}

void VulkanRenderer::Render()
{

    vkWaitForFences(vulkanBase->device, 1, &vulkanBase->gfxQueueFinished, VK_TRUE, UINT64_MAX);
    vkResetFences(vulkanBase->device, 1, &vulkanBase->gfxQueueFinished);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(vulkanBase->device, vulkanBase->swapchain, UINT64_MAX, vulkanBase->imgReady, VK_NULL_HANDLE, &imageIndex);

    VkCommandBufferBeginInfo cmdBuffInfo = {};
    cmdBuffInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBuffInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkResetCommandBuffer(vulkanBase->cmdBuffer, 0);
    vkBeginCommandBuffer(vulkanBase->cmdBuffer, &cmdBuffInfo);
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vulkanBase->renderPass;
    renderPassInfo.framebuffer = vulkanBase->swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = vulkanBase->swapchainInfo.capabilities.currentExtent;
    VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    vkCmdBeginRenderPass(vulkanBase->cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(vulkanBase->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = vulkanBase->swapchainInfo.capabilities.currentExtent.width;
    viewport.height = vulkanBase->swapchainInfo.capabilities.currentExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(vulkanBase->cmdBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = vulkanBase->swapchainInfo.capabilities.currentExtent;
    vkCmdSetScissor(vulkanBase->cmdBuffer, 0, 1, &scissor);

    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(vulkanBase->cmdBuffer, 0, 1, &vertexBuffer, offsets);
    vkCmdBindIndexBuffer(vulkanBase->cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(vulkanBase->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descSet, 0, nullptr);

    vkCmdDrawIndexed(vulkanBase->cmdBuffer, 6, 1, 0, 0, 0);

    vkCmdEndRenderPass(vulkanBase->cmdBuffer);
    vkEndCommandBuffer(vulkanBase->cmdBuffer);

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkanBase->cmdBuffer;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &vulkanBase->imgReady;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &vulkanBase->renderingFinished;
    vkQueueSubmit(vulkanBase->graphicsQueue, 1, &submitInfo, vulkanBase->gfxQueueFinished);

    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.swapchainCount = 1;
    info.pSwapchains = &vulkanBase->swapchain;
    info.pImageIndices = &imageIndex;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &vulkanBase->renderingFinished;
    vkQueuePresentKHR(vulkanBase->presentationQueue, &info);
}

VulkanRenderer::~VulkanRenderer()
{
}

void VulkanRenderer::CreateVertexBuffer()
{
    VkBufferCreateInfo buffInfo = {};
    buffInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffInfo.size = sizeof(Vertex) * 4;
    buffInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(vulkanBase->device, &buffInfo, nullptr, &vertexBuffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vulkanBase->device, vertexBuffer, &memRequirements);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vulkanBase->physicalDevice, &memProperties);
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

    vkAllocateMemory(vulkanBase->device, &allocInfo, nullptr, &vbDevMem);
    vkBindBufferMemory(vulkanBase->device, vertexBuffer, vbDevMem, 0);

    void* data;
    vkMapMemory(vulkanBase->device, vbDevMem, 0, buffInfo.size, 0, &data);
    memcpy(data, vertices.data(), (size_t)buffInfo.size);
    vkUnmapMemory(vulkanBase->device, vbDevMem);
}

void VulkanRenderer::CreateIndexBuffer()
{
    VkBufferCreateInfo buffInfo = {};
    buffInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffInfo.size = sizeof(uint16_t) * 6;
    buffInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    buffInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(vulkanBase->device, &buffInfo, nullptr, &indexBuffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vulkanBase->device, indexBuffer, &memRequirements);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vulkanBase->physicalDevice, &memProperties);
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

    vkAllocateMemory(vulkanBase->device, &allocInfo, nullptr, &ibDevMem);
    vkBindBufferMemory(vulkanBase->device, indexBuffer, ibDevMem, 0);

    void* data;
    vkMapMemory(vulkanBase->device, ibDevMem, 0, buffInfo.size, 0, &data);
    memcpy(data, indices.data(), buffInfo.size);
    vkUnmapMemory(vulkanBase->device, ibDevMem);
}


template<typename T>
GLM_FUNC_QUALIFIER glm::mat<4, 4, T, glm::defaultp> perspectiveTest(T fovy, T aspect, T zNear, T zFar)
{
    using namespace glm;
    assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T, defaultp> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = -static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = zFar / (zFar - zNear);
    Result[3][2] = -(zFar * zNear) / (zFar - zNear);
    Result[2][3] = static_cast<T>(1);
    return Result;
}

template<typename T, glm::qualifier Q>
GLM_FUNC_QUALIFIER glm::mat<4, 4, T, Q> lookAtRH23(glm::vec<3, T, Q> const& eye, glm::vec<3, T, Q> const& center, glm::vec<3, T, Q> const& up)
{
    using namespace glm;
    vec<3, T, Q> const f(normalize(center - eye));
    vec<3, T, Q> const s(normalize(cross(f, up)));
    vec<3, T, Q> const u(cross(s, f));

    mat<4, 4, T, Q> Result(1);
    Result[0][0] = s.x;
    Result[1][0] = s.y;
    Result[2][0] = s.z;
    Result[0][1] = u.x;
    Result[1][1] = u.y;
    Result[2][1] = u.z;
    Result[0][2] = f.x;
    Result[1][2] = f.y;
    Result[2][2] = f.z;
    Result[3][0] = -dot(s, eye);
    Result[3][1] = -dot(u, eye);
    Result[3][2] = -dot(f, eye);
    return Result;
}

void VulkanRenderer::CreateUbo()
{
    VkBufferCreateInfo buffInfo = {};
    buffInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffInfo.size = sizeof(UniformBufferObject);
    buffInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(vulkanBase->device, &buffInfo, nullptr, &uboBuffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vulkanBase->device, uboBuffer, &memRequirements);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vulkanBase->physicalDevice, &memProperties);
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

    vkAllocateMemory(vulkanBase->device, &allocInfo, nullptr, &uboDevMem);
    vkBindBufferMemory(vulkanBase->device, uboBuffer, uboDevMem, 0);

    void* data;
    vkMapMemory(vulkanBase->device, uboDevMem, 0, buffInfo.size, 0, &data);
    UniformBufferObject ubo = {};
    //ubo.model = glm::rotate(glm::mat4(1.0f), 0.00f * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = lookAtRH23(glm::vec3(0.0f, 0.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    ubo.proj = glm::perspectiveRH_ZO(glm::radians(45.0f), vulkanBase->swapchainInfo.capabilities.currentExtent.width/
                           (float)vulkanBase->swapchainInfo.capabilities.currentExtent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    auto test = perspectiveTest(glm::radians(45.0f), vulkanBase->swapchainInfo.capabilities.currentExtent.width /
        (float)vulkanBase->swapchainInfo.capabilities.currentExtent.height, 0.1f, 10.0f);

    ubo.proj = test;
    glm::vec4 t = ubo.view * glm::vec4(vertices[0].pos, 1.0f);
    glm::vec4 res  = ubo.proj * t;
    glm::vec4 res2 = res / res.w;

    ubo.model = glm::mat4(1.0f);
    memcpy(data, &ubo, sizeof(UniformBufferObject));
    vkUnmapMemory(vulkanBase->device, uboDevMem);
}

void VulkanRenderer::CreateGraphicsPipeline()
{
    std::vector<char> vertSrc = readFile("vert.spv");
    std::vector<char> fragSrc = readFile("frag.spv");

    VkShaderModule vsModule, fsModule;
    VkShaderModuleCreateInfo moduleInfo = {};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = vertSrc.size();
    moduleInfo.pCode = (uint32_t*)vertSrc.data();
    vkCreateShaderModule(vulkanBase->device, &moduleInfo, nullptr, &vsModule);
    moduleInfo.codeSize = fragSrc.size();
    moduleInfo.pCode = (uint32_t*)fragSrc.data();
    vkCreateShaderModule(vulkanBase->device, &moduleInfo, nullptr, &fsModule);

    VkPipelineShaderStageCreateInfo shaderInfo[2] = {};
    shaderInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderInfo[0].module = vsModule;
    shaderInfo[0].pName = "main";

    shaderInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderInfo[1].module = fsModule;
    shaderInfo[1].pName = "main";

    VkVertexInputBindingDescription vertBind = {};
    vertBind.binding = 0;
    vertBind.stride = sizeof(Vertex);
    vertBind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertAttr[2] = {};
    vertAttr[0].binding = 0;
    vertAttr[0].location = 0;
    vertAttr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertAttr[0].offset = offsetof(Vertex, pos);

    vertAttr[1].binding = 0;
    vertAttr[1].location = 1;
    vertAttr[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertAttr[1].offset = offsetof(Vertex, color);

    VkPipelineVertexInputStateCreateInfo inputStateInfo = {};
    inputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    inputStateInfo.vertexBindingDescriptionCount = 1;
    inputStateInfo.pVertexBindingDescriptions = &vertBind;
    inputStateInfo.vertexAttributeDescriptionCount = 2;
    inputStateInfo.pVertexAttributeDescriptions = vertAttr;

    VkPipelineInputAssemblyStateCreateInfo inputAss = {};
    inputAss.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAss.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    //inputAss.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    inputAss.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo vpInfo = {};
    vpInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpInfo.viewportCount = 1;
    vpInfo.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterInfo = {};
    rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterInfo.depthClampEnable = VK_FALSE;
    rasterInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    //rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterInfo.depthBiasEnable = VK_FALSE;
    rasterInfo.depthBiasConstantFactor = 0.0f; // Optional
    rasterInfo.depthBiasClamp = 0.0f; // Optional
    rasterInfo.depthBiasSlopeFactor = 0.0f; // Optional
    rasterInfo.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional


    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo blendInfo = {};
    blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendInfo.logicOpEnable = VK_FALSE;
    blendInfo.logicOp = VK_LOGIC_OP_COPY;
    blendInfo.attachmentCount = 1;
    blendInfo.pAttachments = &colorBlendAttachment;
    blendInfo.blendConstants[0] = 0.0f;
    blendInfo.blendConstants[1] = 0.0f;
    blendInfo.blendConstants[2] = 0.0f;
    blendInfo.blendConstants[3] = 0.0f;
         
    VkDynamicState dynamicStates[2] = {
                    VK_DYNAMIC_STATE_VIEWPORT,
                    VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderInfo;
    pipelineInfo.pVertexInputState = &inputStateInfo;
    pipelineInfo.pInputAssemblyState = &inputAss;
    pipelineInfo.pViewportState = &vpInfo;
    pipelineInfo.pRasterizationState = &rasterInfo;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &blendInfo;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = vulkanBase->renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex =  -1;

    vkCreateGraphicsPipelines(vulkanBase->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
}

void VulkanRenderer::CreatePipelineLayout()
{
    VkDescriptorSetLayoutBinding bindings = {};
    bindings.binding = 0;
    bindings.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings.descriptorCount = 1;
    bindings.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descSetLayoutInfo = {};
    descSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descSetLayoutInfo.bindingCount = 1;
    descSetLayoutInfo.pBindings = &bindings;
    vkCreateDescriptorSetLayout(vulkanBase->device, &descSetLayoutInfo, nullptr, &descSetLayout);

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &descSetLayout;
    layoutInfo.pushConstantRangeCount = 0;
    layoutInfo.pPushConstantRanges = nullptr;
    vkCreatePipelineLayout(vulkanBase->device, &layoutInfo, nullptr, &pipelineLayout);
}

void VulkanRenderer::CreatePoolAndSets()
{
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolDesc = {};
    poolDesc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolDesc.maxSets = 1;
    poolDesc.poolSizeCount = 1;
    poolDesc.pPoolSizes = &poolSize;
    vkCreateDescriptorPool(vulkanBase->device, &poolDesc, nullptr, &descPool);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descSetLayout;
    vkAllocateDescriptorSets(vulkanBase->device, &allocInfo, &descSet);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uboBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(vulkanBase->device, 1, &descriptorWrite, 0, nullptr);
}

std::vector<char> VulkanRenderer::readFile(const std::string& filename)
{
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
