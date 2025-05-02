#define RIGHT_FACE 4
#define LEFT_FACE 8
#define BACK_FACE 12
#define BOTTOM_FACE 16
#define TOP_FACE 20
#include "VulkanRenderer.hpp"
#include "VulkanOps.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include "ImageFile.h"
using namespace std;
struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
};

const std::vector<Vertex> vertices = {
    // front face
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{0.5f, -0.5f,  -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{-0.5f, -0.5f,  -0.5f}, {1.0f, 1.0f, 1.0f}},
    // right face
    {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
    {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}},
    {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
    // left face
    {{-0.5f, 0.5f,  -0.5f}, {1.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},
    {{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
    // back face
    {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    // bottom face
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    // top face
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},

};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
    0 + RIGHT_FACE, 1 + RIGHT_FACE, 2 + RIGHT_FACE, 2 + RIGHT_FACE, 3 + RIGHT_FACE, 0 + RIGHT_FACE,
    2 + LEFT_FACE, 1 + LEFT_FACE, 0 + LEFT_FACE, 0 + LEFT_FACE, 3 + LEFT_FACE, 2 + LEFT_FACE,
    2 + BACK_FACE, 1 + BACK_FACE, 0 + BACK_FACE, 0 + BACK_FACE, 3 + BACK_FACE, 2 + BACK_FACE,
    0 + BOTTOM_FACE, 1 + BOTTOM_FACE,2 + BOTTOM_FACE,  1 + BOTTOM_FACE, 3 + BOTTOM_FACE,  2 + BOTTOM_FACE,
    2+ TOP_FACE, 1 + TOP_FACE, 0 + TOP_FACE, 2 + TOP_FACE, 3 + TOP_FACE, 1 + TOP_FACE,
};


struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 lightPos; // w component is 
    glm::vec4 lightCol; // w component is 
    //matrix proj;
    char alignment[256 - 224];
};

template<typename T>
GLM_FUNC_QUALIFIER glm::mat<4, 4, T, glm::defaultp> perspectiveTest(T fovy, T aspect, T zNear, T zFar)
{
    using namespace glm;
    assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T, defaultp> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = -static_cast<T>(1) / (tanHalfFovy); // flipped axes
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

VulkanRenderer::VulkanRenderer(HINSTANCE hinstance, HWND hwnd)
{
    vulkanBase = createVulkanBase(hinstance, hwnd);
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateUbo();
    CreateSampler();
    PrepareTexture();
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
    VkClearValue clearColor[2] = {};
    clearColor[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearColor[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearColor;
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
    vkCmdDrawIndexed(vulkanBase->cmdBuffer, indices.size(), 1, 0, 0, 0);

    vkCmdBindDescriptorSets(vulkanBase->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descSet2, 0, nullptr);
    vkCmdDrawIndexed(vulkanBase->cmdBuffer, indices.size(), 1, 0, 0, 0);

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

void VulkanRenderer::updateRotation()
{
    UniformBufferObject ubo = {};
    angle = 0;
// first box
    ubo.model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1.0f, 0.0f, 0.0f)) * ubo.model;
    ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 3.0f)) * ubo.model;
    ubo.view = glm::lookAtLH(glm::vec3(4.0f, 2.0f, -2.0f), glm::vec3(0.0f, 0.0f, 4.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.proj = perspectiveTest(glm::radians(45.0f), vulkanBase->swapchainInfo.capabilities.currentExtent.width /
        (float)vulkanBase->swapchainInfo.capabilities.currentExtent.height, 0.1f, 10.0f);
    ubo.proj = glm::perspectiveLH_ZO(glm::radians(45.0f), vulkanBase->swapchainInfo.capabilities.currentExtent.width /
        (float)vulkanBase->swapchainInfo.capabilities.currentExtent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;
    ubo.lightPos = glm::vec4(0.0f, 0.0f, -4.0f, 0.0f);
    ubo.lightCol = glm::vec4(0.7f, 0.7f, 0.4f, 0.47f);
    memcpy(uboData, &ubo, sizeof(UniformBufferObject));

    ubo.model = glm::rotate(glm::mat4(1.0f), -angle, glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.model = glm::rotate(glm::mat4(1.0f), -angle, glm::vec3(1.0f, 0.0f, 0.0f)) * ubo.model;
    ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 6.0f)) * ubo.model;
    memcpy((char*)uboData + sizeof(UniformBufferObject), &ubo, sizeof(UniformBufferObject));
}

VulkanRenderer::~VulkanRenderer()
{
}

void VulkanRenderer::CreateVertexBuffer()
{
    VkBufferCreateInfo buffInfo = {};
    buffInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffInfo.size = sizeof(Vertex) * vertices.size();
    buffInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(vulkanBase->device, &buffInfo, nullptr, &vertexBuffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vulkanBase->device, vertexBuffer, &memRequirements);
    vbDevMem = allocateBuffer(vulkanBase->device, vulkanBase->physicalDevice, memRequirements, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
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
    buffInfo.size = sizeof(uint16_t) * indices.size();
    buffInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    buffInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(vulkanBase->device, &buffInfo, nullptr, &indexBuffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vulkanBase->device, indexBuffer, &memRequirements);
    ibDevMem = allocateBuffer(vulkanBase->device, vulkanBase->physicalDevice, memRequirements, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    vkBindBufferMemory(vulkanBase->device, indexBuffer, ibDevMem, 0);

    void* data;
    vkMapMemory(vulkanBase->device, ibDevMem, 0, buffInfo.size, 0, &data);
    memcpy(data, indices.data(), buffInfo.size);
    vkUnmapMemory(vulkanBase->device, ibDevMem);
}

void VulkanRenderer::CreateUbo()
{
    VkBufferCreateInfo buffInfo = {};
    buffInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffInfo.size = sizeof(UniformBufferObject);
    buffInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(vulkanBase->device, &buffInfo, nullptr, &uboBuffer);
    vkCreateBuffer(vulkanBase->device, &buffInfo, nullptr, &uboBuffer2);

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
    allocInfo.allocationSize = memRequirements.size * 2;
    allocInfo.memoryTypeIndex = i;

    vkAllocateMemory(vulkanBase->device, &allocInfo, nullptr, &uboDevMem);
    vkBindBufferMemory(vulkanBase->device, uboBuffer, uboDevMem, 0);
    vkBindBufferMemory(vulkanBase->device, uboBuffer2, uboDevMem, memRequirements.size);
    vkMapMemory(vulkanBase->device, uboDevMem, 0, buffInfo.size, 0, &uboData);
}

void VulkanRenderer::CreateGraphicsPipeline()
{
    std::vector<char> vertSrc = readFile("vert.spv");
    std::vector<char> fragSrc = readFile("frag.spv");

    VkShaderModule vsModule, fsModule;
    shaderc_compiler_t compiler = shaderc_compiler_initialize();
    fsModule = compileShader(vulkanBase->device, nullptr, "09_shader_base.frag", "main", shaderc_fragment_shader, compiler);
    vsModule = compileShader(vulkanBase->device, nullptr, "09_shader_base.vert", "main", shaderc_vertex_shader, compiler);
    shaderc_compiler_release(compiler);


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

    VkPipelineDepthStencilStateCreateInfo depthInfo = {};
    depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthInfo.depthTestEnable = VK_TRUE;
    depthInfo.depthWriteEnable = VK_TRUE;
    depthInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthInfo.depthBoundsTestEnable = VK_FALSE;
    depthInfo.stencilTestEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderInfo;
    pipelineInfo.pVertexInputState = &inputStateInfo;
    pipelineInfo.pInputAssemblyState = &inputAss;
    pipelineInfo.pViewportState = &vpInfo;
    pipelineInfo.pRasterizationState = &rasterInfo;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthInfo; // Optional
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
    VkDescriptorSetLayoutBinding bindings[2] = {};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].pImmutableSamplers = nullptr;


    VkDescriptorSetLayoutCreateInfo descSetLayoutInfo = {};
    descSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descSetLayoutInfo.bindingCount = 2;
    descSetLayoutInfo.pBindings = bindings;
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
    VkDescriptorPoolSize poolSize[2] = {};
    poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize[0].descriptorCount = 2;
    poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize[1].descriptorCount = 2;


    VkDescriptorPoolCreateInfo poolDesc = {};
    poolDesc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolDesc.maxSets = 2;
    poolDesc.poolSizeCount = 2;
    poolDesc.pPoolSizes = poolSize;
    vkCreateDescriptorPool(vulkanBase->device, &poolDesc, nullptr, &descPool);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descSetLayout;
    vkAllocateDescriptorSets(vulkanBase->device, &allocInfo, &descSet);
    vkAllocateDescriptorSets(vulkanBase->device, &allocInfo, &descSet2);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uboBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imgInfo = {};
    imgInfo.imageView = tex.texView;
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imgInfo.sampler = sampler;

    VkWriteDescriptorSet descriptorWrite[2] = {};
    descriptorWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[0].dstSet = descSet;
    descriptorWrite[0].dstBinding = 0;
    descriptorWrite[0].dstArrayElement = 0;
    descriptorWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite[0].descriptorCount = 1;
    descriptorWrite[0].pBufferInfo = &bufferInfo;

    descriptorWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[1].dstSet = descSet;
    descriptorWrite[1].dstBinding = 1;
    descriptorWrite[1].dstArrayElement = 0;
    descriptorWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite[1].descriptorCount = 1;
    descriptorWrite[1].pImageInfo = &imgInfo;
    vkUpdateDescriptorSets(vulkanBase->device, 2, descriptorWrite, 0, nullptr);

    bufferInfo.buffer = uboBuffer2;
    descriptorWrite[0].dstSet = descSet2;
    descriptorWrite[1].dstSet = descSet2;
    vkUpdateDescriptorSets(vulkanBase->device, 2, descriptorWrite, 0, nullptr);
}

void VulkanRenderer::CreateSampler()
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(vulkanBase->physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    CHECK_VK_RESULT(vkCreateSampler(vulkanBase->device, &samplerInfo, nullptr, &sampler));
}

void VulkanRenderer::PrepareTexture()
{
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMem;
    void* stagingPtr;
    ImageFile texImage(L"texture.png");
    tex = create2DTexture(vulkanBase->device, vulkanBase->physicalDevice, texImage.GetWidth(), texImage.GetHeight(), VK_FORMAT_R8G8B8A8_UNORM);
    


    VkBufferCreateInfo buffInfo = {};
    buffInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffInfo.size = texImage.GetWidth() *  texImage.GetHeight() * 4;
    buffInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(vulkanBase->device, &buffInfo, nullptr, &stagingBuffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vulkanBase->device, stagingBuffer, &memRequirements);
    stagingBufferMem = allocateBuffer(vulkanBase->device, vulkanBase->physicalDevice, memRequirements, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    vkBindBufferMemory(vulkanBase->device, stagingBuffer, stagingBufferMem, 0);

    vkMapMemory(vulkanBase->device, stagingBufferMem, 0, memRequirements.size, 0, &stagingPtr);
    memcpy(stagingPtr, texImage.GetFilePtr(), buffInfo.size);
    vkUnmapMemory(vulkanBase->device, stagingBufferMem);

    // copy buffer to memory and transition all resources
    VkCommandBufferBeginInfo cmdBuffInfo = {};
    cmdBuffInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBuffInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkResetCommandBuffer(vulkanBase->cmdBuffer, 0);
    vkBeginCommandBuffer(vulkanBase->cmdBuffer, &cmdBuffInfo);


    VkImageMemoryBarrier copyBarrier = {};
    copyBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    copyBarrier.srcAccessMask = 0;
    copyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
    copyBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    copyBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    copyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copyBarrier.image = tex.texImage;
    copyBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyBarrier.subresourceRange.baseMipLevel = 0;
    copyBarrier.subresourceRange.levelCount = 1;
    copyBarrier.subresourceRange.baseArrayLayer = 0;
    copyBarrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(vulkanBase->cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &copyBarrier);

    VkBufferImageCopy imgCopy = {};
    imgCopy.bufferOffset = 0;
    imgCopy.bufferRowLength = 0;
    imgCopy.bufferImageHeight = 0;
    imgCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imgCopy.imageSubresource.mipLevel = 0;
    imgCopy.imageSubresource.baseArrayLayer = 0;
    imgCopy.imageSubresource.layerCount = 1;
    imgCopy.imageOffset = { 0, 0, 0 };
    imgCopy.imageExtent = {
        (unsigned int)texImage.GetWidth(),
        (unsigned int)texImage.GetHeight(),
        1
    };

    vkCmdCopyBufferToImage(vulkanBase->cmdBuffer, stagingBuffer, tex.texImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopy);

    VkImageMemoryBarrier layoutBarrier = {};
    layoutBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    layoutBarrier.srcAccessMask = 0;
    layoutBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    layoutBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    layoutBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    layoutBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    layoutBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    layoutBarrier.image = tex.texImage;
    layoutBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    layoutBarrier.subresourceRange.baseMipLevel = 0;
    layoutBarrier.subresourceRange.levelCount = 1;
    layoutBarrier.subresourceRange.baseArrayLayer = 0;
    layoutBarrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(vulkanBase->cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &layoutBarrier);


    vkEndCommandBuffer(vulkanBase->cmdBuffer);
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkanBase->cmdBuffer;
    vkQueueSubmit(vulkanBase->graphicsQueue, 1, &submitInfo, nullptr);
    CHECK_VK_RESULT(vkQueueWaitIdle(vulkanBase->graphicsQueue));

    vkDestroyBuffer(vulkanBase->device, stagingBuffer, nullptr);
    vkFreeMemory(vulkanBase->device, stagingBufferMem, nullptr);

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
