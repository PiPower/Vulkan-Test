#define _CRT_SECURE_NO_WARNINGS 1
#define RIGHT_FACE 4
#define LEFT_FACE 8
#define BACK_FACE 12
#define BOTTOM_FACE 16
#define TOP_FACE 20
#include "VulkanRenderer.hpp"
#include "VulkanOps.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <fstream>
#define TEXTURE_FORMAT VK_FORMAT_R8G8B8A8_SRGB
#include <locale>
#include <codecvt>
#include <string>

using namespace std;
struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 tex;
};
#pragma pack(1)
struct ComputeMetadata
{
    int resX;
    int resY;
    int padd[2];
};

uint32_t faceCount;

const std::vector<Vertex> vertices = {
    // front face
    {{-0.5f, 0.5f, -0.5f}, {0, 0, -1.0f}, {0.69f, 0.0f}},
    {{0.5f, 0.5f, -0.5f},  {0, 0, -1.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f,  -0.5f},  {0, 0, -1.0f}, {1.0f, 1.0f}},
    {{-0.5f, -0.5f,  -0.5f},  {0, 0, -1.0f}, {0.69f, 1.0f}},
    // right face
    {{0.5f, 0.5f, -0.5f}, {1.0f, 0, 0}, {0.0f, 0.0f}},
    {{0.5f, 0.5f, 0.5f}, {1.0f, 0, 0}, {0.33f, 0.0f}},
    {{0.5f, -0.5f, 0.5f}, {1.0f, 0, 0}, {0.33f, 1.0f}},
    {{0.5f, -0.5f, -0.5f}, {1.0f, 0, 0}, {0.0f, 1.0f}},
    // left face
    {{-0.5f, 0.5f,  -0.5f}, {-1.0f, 0, 0}},
    {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0, 0}},
    {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0, 0}},
    {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0, 0}},
    // back face
    {{-0.5f, 0.5f, 0.5f},  {0, 0, 1.0f}},
    {{0.5f, 0.5f, 0.5f}, {0, 0, 1.0f}},
    {{0.5f, -0.5f, 0.5f}, {0, 0, 1.0f}},
    {{-0.5f, -0.5f, 0.5f}, {0, 0, 1.0f}},
    // bottom face
    {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}},
    {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}},
    {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}},
    // top face
    {{-0.5f, 0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.5f},{0.0f, 1.0f, 0.0f}, {0.33f, 0.0f}},
    {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.33f, 1.0f}},

};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
    0 + RIGHT_FACE, 1 + RIGHT_FACE, 2 + RIGHT_FACE, 2 + RIGHT_FACE, 3 + RIGHT_FACE, 0 + RIGHT_FACE,
    2 + LEFT_FACE, 1 + LEFT_FACE, 0 + LEFT_FACE, 0 + LEFT_FACE, 3 + LEFT_FACE, 2 + LEFT_FACE,
    2 + BACK_FACE, 1 + BACK_FACE, 0 + BACK_FACE, 0 + BACK_FACE, 3 + BACK_FACE, 2 + BACK_FACE,
    0 + BOTTOM_FACE, 1 + BOTTOM_FACE,2 + BOTTOM_FACE,  1 + BOTTOM_FACE, 3 + BOTTOM_FACE,  2 + BOTTOM_FACE,
    2+ TOP_FACE, 1 + TOP_FACE, 0 + TOP_FACE, 2 + TOP_FACE, 3 + TOP_FACE, 1 + TOP_FACE,
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

VulkanRenderer::VulkanRenderer(HINSTANCE hinstance, HWND hwnd, const std::string& path)
    :
    firstRun(true)
{
    vulkanBase = createVulkanBase(hinstance, hwnd);
    CreateComputeLayout();
    CreateComputePipeline();
    loadScene(path);
    CreateSampler();
    CreatePipelineLayout();
    CreateGraphicsPipeline();
    CreateUbo();
    CreatePoolAndSets();
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
    renderPassInfo.framebuffer = vulkanBase->swapchainFramebuffers[0];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = vulkanBase->swapchainInfo.capabilities.currentExtent;
    VkClearValue clearColor[2] = {};
    clearColor[1].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearColor[0].depthStencil = { 1.0f, 0 };

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
    vkCmdBindVertexBuffers(vulkanBase->cmdBuffer, 0, 1, &sceneGeometry.vertexBuffer, offsets);
    vkCmdBindIndexBuffer(vulkanBase->cmdBuffer, sceneGeometry.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    for (size_t item = 0; item < renderableItems.size() ; item++)
    {
        DrawItem(item);
    }

    vkCmdEndRenderPass(vulkanBase->cmdBuffer);

    VkImageMemoryBarrier copyToSwap = {};
    copyToSwap.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    copyToSwap.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    copyToSwap.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    copyToSwap.oldLayout = firstRun ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    copyToSwap.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    copyToSwap.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copyToSwap.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copyToSwap.image = vulkanBase->swapchainImages[imageIndex];
    copyToSwap.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyToSwap.subresourceRange.baseMipLevel = 0;
    copyToSwap.subresourceRange.levelCount = 1;
    copyToSwap.subresourceRange.baseArrayLayer = 0;
    copyToSwap.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(vulkanBase->cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &copyToSwap);

    VkImageCopy copyDesc = {};
    copyDesc.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyDesc.srcSubresource.baseArrayLayer = 0;
    copyDesc.srcSubresource.layerCount = 1;
    copyDesc.srcSubresource.mipLevel = 0;
    copyDesc.srcOffset = { 0, 0, 0 };
    copyDesc.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyDesc.dstSubresource.baseArrayLayer = 0;
    copyDesc.dstSubresource.layerCount = 1;
    copyDesc.dstSubresource.mipLevel = 0;
    copyDesc.dstOffset = { 0, 0, 0 };
    copyDesc.extent = { vulkanBase->swapchainInfo.capabilities.currentExtent.width, vulkanBase->swapchainInfo.capabilities.currentExtent.height, 1 };
    vkCmdCopyImage(vulkanBase->cmdBuffer, vulkanBase->renderTexture.texImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        vulkanBase->swapchainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyDesc);


    VkImageMemoryBarrier copyToPresent = {};
    copyToPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    copyToPresent.srcAccessMask = 0;
    copyToPresent.dstAccessMask = 0;
    copyToPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    copyToPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    copyToPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copyToPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copyToPresent.image = vulkanBase->swapchainImages[imageIndex];
    copyToPresent.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyToPresent.subresourceRange.baseMipLevel = 0;
    copyToPresent.subresourceRange.levelCount = 1;
    copyToPresent.subresourceRange.baseArrayLayer = 0;
    copyToPresent.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(vulkanBase->cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &copyToPresent);

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

    vkQueueWaitIdle(vulkanBase->presentationQueue);

    if (imageIndex == 2)
    {
        firstRun = false;
    }
}

void VulkanRenderer::updateRotation()
{
    angle += 0.0001;
    // first box
    char* uboPerObj = (char*)uboData + uboGlobalProps.size + uboComputeProps.size;

    for (size_t item = 0; item < renderableItems.size(); item++)
    {
        uint32_t uboOffset = item * uboPerObjProps.size;
        renderableItems[item].uboOffset = uboOffset;
        memcpy(uboPerObj + uboOffset, &renderableItems[item].transformation, sizeof(PerObjUbo));
    }

    /*
    for (int z = 0; z < SQUARE_COUNT_Z; z++)
    {
        for (int y = 0; y < SQUARE_COUNT_Y; y++)
        {
            for (int x = 0; x < SQUARE_COUNT_X; x++)
            {
                PerObjUbo perObjUbo = {};
                perObjUbo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
                char* objBuffPtr = (char*)uboData + uboGlobalProps.size + (z * SQUARE_COUNT_Y * SQUARE_COUNT_X + y * SQUARE_COUNT_X + x) * uboPerObjProps.size;
                memcpy(objBuffPtr, &perObjUbo, sizeof(PerObjUbo));
            }
        }
    }

    PerObjUbo perObjUbo = {};
    perObjUbo.model = glm::mat4(1.0f);
    memcpy(uboData, &perObjUbo, sizeof(PerObjUbo));*/
}

void VulkanRenderer::updateCameraLH(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up)
{
    GlobalUbo globalUbo = {};

    globalUbo.view = glm::lookAtLH(eye, center, up);
    globalUbo.proj = glm::perspectiveLH_ZO(glm::radians(45.0f), vulkanBase->swapchainInfo.capabilities.currentExtent.width /
        (float)vulkanBase->swapchainInfo.capabilities.currentExtent.height, 0.1f, 90.0f);
    globalUbo.proj[1][1] *= -1;
    globalUbo.lightPos = glm::vec4(0.0f, 2.0f,0.0f, 0.0f);
    globalUbo.lightCol = glm::vec4(0.9f, 0.9f, 0.9f, 0.1f); // (colx, coly, colz, ambient factor)
    memcpy(uboData, &globalUbo, sizeof(GlobalUbo));
}


void VulkanRenderer::loadScene(const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder);

    size_t vertexCount = 0;
    size_t indexCount = 0;
    uint32_t imageArrayOffset = 0;
    for (uint32_t i = 0; i < scene->mNumMaterials; i++)
    {
        aiMaterial* processedMaterial = scene->mMaterials[i];
        aiString str;
        aiReturn ret = processedMaterial->GetTexture(aiTextureType_BASE_COLOR, 0, &str);
        if (ret != aiReturn_SUCCESS)
        {
            materialTextureIdx.push_back(0);
        }
        else
        {
            materialTextureIdx.push_back(imageArrayOffset);
            imageArrayOffset++;
        }

    }
    for (int i = 0; i < scene->mNumMeshes; i++)
    {
        if (scene->mMeshes[i]->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
        {
            OutputDebugString(L"Unsupported mesh type");
            exit(-1);
        }
        vertexCount += scene->mMeshes[i]->mNumVertices;
        indexCount += scene->mMeshes[i]->mNumFaces * 3;
    }

    if (vertexCount * sizeof(Vertex) > 4'000'000'000 ||
        indexCount * sizeof(uint32_t) > 4'000'000'000)
    {
        OutputDebugString(L"Scene is too large");
        exit(-1);
    }
    VkBuffer stagingVB, stagingIB;
    VkDeviceMemory stagingVbDevMem, stagingIbDevMem;
    VkBufferCreateInfo buffInfoVB = {};
    buffInfoVB.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffInfoVB.size = vertexCount * sizeof(Vertex);
    buffInfoVB.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffInfoVB.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(vulkanBase->device, &buffInfoVB, nullptr, &sceneGeometry.vertexBuffer);
    vkCreateBuffer(vulkanBase->device, &buffInfoVB, nullptr, &stagingVB);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(vulkanBase->device, sceneGeometry.vertexBuffer, &memReqs);
    sceneGeometry.vbDevMem = allocateBuffer(vulkanBase->device, vulkanBase->physicalDevice, memReqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    stagingVbDevMem = allocateBuffer(vulkanBase->device, vulkanBase->physicalDevice, memReqs, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    vkBindBufferMemory(vulkanBase->device, sceneGeometry.vertexBuffer, sceneGeometry.vbDevMem, 0);
    vkBindBufferMemory(vulkanBase->device, stagingVB, stagingVbDevMem, 0);


    VkBufferCreateInfo buffInfoIB = {};
    buffInfoIB.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffInfoIB.size = indexCount * sizeof(uint32_t);
    buffInfoIB.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffInfoIB.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(vulkanBase->device, &buffInfoIB, nullptr, &sceneGeometry.indexBuffer);
    vkCreateBuffer(vulkanBase->device, &buffInfoIB, nullptr, &stagingIB);
    vkGetBufferMemoryRequirements(vulkanBase->device, sceneGeometry.indexBuffer, &memReqs);
    sceneGeometry.ibDevMem = allocateBuffer(vulkanBase->device, vulkanBase->physicalDevice, memReqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    stagingIbDevMem = allocateBuffer(vulkanBase->device, vulkanBase->physicalDevice, memReqs, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    vkBindBufferMemory(vulkanBase->device, stagingIB, stagingIbDevMem, 0);
    vkBindBufferMemory(vulkanBase->device, sceneGeometry.indexBuffer, sceneGeometry.ibDevMem, 0);
    
    char* dataIB, *dataVB;
    vkMapMemory(vulkanBase->device, stagingVbDevMem, 0, buffInfoVB.size, 0, (void**) & dataVB);
    vkMapMemory(vulkanBase->device, stagingIbDevMem, 0, buffInfoIB.size, 0, (void**)&dataIB);
    uint32_t vertCount = 0;
    for (int i = 0; i < scene->mNumMeshes; i++)
    {
        for (int j = 0; j < scene->mMeshes[i]->mNumVertices; j++)
        {
            Vertex vert = {};
            vert.pos.y = scene->mMeshes[i]->mVertices[j].y;
            vert.pos.z = scene->mMeshes[i]->mVertices[j].z;
            vert.pos.x = scene->mMeshes[i]->mVertices[j].x;

            vert.normal.x = scene->mMeshes[i]->mNormals[j].x;
            vert.normal.y = scene->mMeshes[i]->mNormals[j].y;
            vert.normal.z = scene->mMeshes[i]->mNormals[j].z;

            vert.tex.x = scene->mMeshes[i]->mTextureCoords[0][j].x;
            vert.tex.y = scene->mMeshes[i]->mTextureCoords[0][j].y;
            memcpy(dataVB + vertCount * sizeof(Vertex) + j * sizeof(Vertex), &vert, sizeof(Vertex));
        }

        for (int j = 0; j < scene->mMeshes[i]->mNumFaces; j++)
        {
            uint32_t face[3];
            face[0] = scene->mMeshes[i]->mFaces[j].mIndices[0];
            face[1] = scene->mMeshes[i]->mFaces[j].mIndices[1];
            face[2] = scene->mMeshes[i]->mFaces[j].mIndices[2];
            memcpy(dataIB + faceCount * 3 * sizeof(uint32_t) + j * sizeof(uint32_t) * 3, face, sizeof(uint32_t) * 3);
        }

        sceneGeometry.vbOffset.push_back(vertCount);
        sceneGeometry.ibOffset.push_back(faceCount * 3);
        sceneGeometry.indexCount.push_back(scene->mMeshes[i]->mNumFaces * 3);
        sceneGeometry.materialIndex.push_back(scene->mMeshes[i]->mMaterialIndex);
        vertCount += scene->mMeshes[i]->mNumVertices;
        faceCount += scene->mMeshes[i]->mNumFaces;
    }

    vkUnmapMemory(vulkanBase->device, stagingVbDevMem);
    vkUnmapMemory(vulkanBase->device, stagingIbDevMem);

    VkBufferCopy ibRegion = {}, vbRegion = {};
    vbRegion.srcOffset = 0;
    vbRegion.dstOffset = 0;
    vbRegion.size = buffInfoVB.size;

    ibRegion.srcOffset = 0;
    ibRegion.dstOffset = 0;
    ibRegion.size = buffInfoIB.size;

    VkCommandBufferBeginInfo cmdBuffInfo = {};
    cmdBuffInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBuffInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkResetCommandBuffer(vulkanBase->cmdBuffer, 0);
    vkBeginCommandBuffer(vulkanBase->cmdBuffer, &cmdBuffInfo);

    vkCmdCopyBuffer(vulkanBase->cmdBuffer, stagingVB, sceneGeometry.vertexBuffer, 1, &vbRegion);
    vkCmdCopyBuffer(vulkanBase->cmdBuffer, stagingIB, sceneGeometry.indexBuffer, 1, &ibRegion);

    vkEndCommandBuffer(vulkanBase->cmdBuffer);
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkanBase->cmdBuffer;
    vkQueueSubmit(vulkanBase->graphicsQueue, 1, &submitInfo, nullptr);
    vkQueueWaitIdle(vulkanBase->graphicsQueue);

    vkDestroyBuffer(vulkanBase->device, stagingVB, nullptr);
    vkDestroyBuffer(vulkanBase->device, stagingIB, nullptr);
    vkFreeMemory(vulkanBase->device, stagingVbDevMem, nullptr);
    vkFreeMemory(vulkanBase->device, stagingIbDevMem, nullptr);

    // -------------------
    // Read structure info 
    // -------------------
    parseObjectTree(scene->mRootNode, glm::mat4(1.0f));

    // -------------------
    // Read material info  async
    // -------------------

    TextureArray array(100);
    string cachePath = path.substr(0, path.find_last_of('\\'));

    cachePath += "\\textures\\textureArray.cache";
    if (!array.loadFromFile(cachePath))
    {
        vector<ImageFile*> images = GenerateTextureArrayCache(scene->mMaterials, scene->mNumMaterials, path, cachePath);
        for (int i = 0; i < images.size(); i++)
        {
            delete images[i];
        }
        array.loadFromFile(cachePath);
    }
    // -------------------
    // Uploading materials to GPU
    // -------------------
    UploadImages(array);
}

void VulkanRenderer::parseObjectTree(aiNode* node, const glm::mat4x4& transform)
{
    glm::mat4x4 localTransform;
    memcpy(&localTransform, &node->mTransformation, sizeof(float) * 4 * 4);
    localTransform = glm::transpose(localTransform);// assimp stores transforms in row major
    localTransform = localTransform * transform;
    if (node->mNumMeshes != 0)
    {
        Object obj = {};
        obj.transformation.model = localTransform;
        obj.meshIdx.resize(node->mNumMeshes);
        for (size_t i = 0; i < node->mNumMeshes; i++)
        {
            obj.meshIdx[i] = node->mMeshes[i];
        }
        obj.transformation.index = glm::ivec4();
        obj.name = node->mName.C_Str();
        if (renderableItems.size() == 43)
        {
            int x = 2;
        }
        renderableItems.push_back(std::move(obj));
    }
    for (size_t i = 0; i < node->mNumChildren; i++)
    {
        parseObjectTree(node->mChildren[i], localTransform);
    }
}

VulkanRenderer::~VulkanRenderer()
{

}


void VulkanRenderer::CreateUbo()
{
    VkDeviceSize totalMemorySize = 0;
    uboGlobalProps = getBufferMemoryProperties(vulkanBase->device, sizeof(GlobalUbo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    totalMemorySize += uboGlobalProps.size;
    uboComputeProps = getBufferMemoryProperties(vulkanBase->device, sizeof(ComputeMetadata), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    totalMemorySize += uboComputeProps.size;
    uboPerObjProps = getBufferMemoryProperties(vulkanBase->device, sizeof(PerObjUbo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    totalMemorySize += uboPerObjProps.size * renderableItems.size();

    vector<VkDeviceSize> offsets = { 0, uboGlobalProps.size, uboComputeProps.size + uboGlobalProps.size };
    vector<VkDeviceSize> sizes = { uboGlobalProps.size, uboComputeProps.size, uboPerObjProps.size * renderableItems.size() };
    UniformBuffer ubo = createUniformBuffer(vulkanBase->device, vulkanBase->physicalDevice, totalMemorySize, offsets, sizes);

    vkDestroyBuffer(vulkanBase->device, ubo.globalBuffer, nullptr);
    uboGlobal = ubo.subBuffers[0];
    uboCompute = ubo.subBuffers[1];
    uboPerObj = ubo.subBuffers[2];
    uboDevMem = ubo.buffMem;

    vkMapMemory(vulkanBase->device, uboDevMem, 0, totalMemorySize, 0, &uboData);


}

void VulkanRenderer::CreateGraphicsPipeline()
{
    string textureCount = to_string(materials.size());
    VkShaderModule vsModule, fsModule;
    shaderc_compile_options_t options = shaderc_compile_options_initialize();
    shaderc_compile_options_add_macro_definition(options, "TEXTURE_COUNT", 13, textureCount.c_str(), textureCount.size() - 1);
    shaderc_compiler_t compiler = shaderc_compiler_initialize();


    fsModule = compileShader(vulkanBase->device, nullptr, "09_shader_base.frag", "main", shaderc_fragment_shader, compiler, options);
    vsModule = compileShader(vulkanBase->device, nullptr, "09_shader_base.vert", "main", shaderc_vertex_shader, compiler, options);

    shaderc_compiler_release(compiler);
    shaderc_compile_options_release(options);

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

    VkVertexInputAttributeDescription vertAttr[3] = {};
    vertAttr[0].binding = 0;
    vertAttr[0].location = 0;
    vertAttr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertAttr[0].offset = offsetof(Vertex, pos);

    vertAttr[1].binding = 0;
    vertAttr[1].location = 1;
    vertAttr[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertAttr[1].offset = offsetof(Vertex, normal);

    vertAttr[2].binding = 0;
    vertAttr[2].location = 2;
    vertAttr[2].format = VK_FORMAT_R32G32_SFLOAT;
    vertAttr[2].offset = offsetof(Vertex, tex);

    VkPipelineVertexInputStateCreateInfo inputStateInfo = {};
    inputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    inputStateInfo.vertexBindingDescriptionCount = 1;
    inputStateInfo.pVertexBindingDescriptions = &vertBind;
    inputStateInfo.vertexAttributeDescriptionCount = 3;
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
    //rasterInfo.polygonMode = VK_POLYGON_MODE_LINE;
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
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Optional
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

    vkDestroyShaderModule(vulkanBase->device, fsModule, nullptr);
    vkDestroyShaderModule(vulkanBase->device, vsModule, nullptr);
}

void VulkanRenderer::CreatePipelineLayout()
{
    VkDescriptorSetLayoutBinding bindings[3] = {};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = materials.size();
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].pImmutableSamplers = nullptr;

    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[2].pImmutableSamplers = nullptr;


    VkDescriptorSetLayoutCreateInfo descSetLayoutInfo = {};
    descSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descSetLayoutInfo.bindingCount = 3;
    descSetLayoutInfo.pBindings = bindings;
    vkCreateDescriptorSetLayout(vulkanBase->device, &descSetLayoutInfo, nullptr, &descSetLayout);

    VkPushConstantRange range = {};
    range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    range.offset = 0;
    range.size = sizeof(int) * 4;

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &descSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &range;
    vkCreatePipelineLayout(vulkanBase->device, &layoutInfo, nullptr, &pipelineLayout);
}

void VulkanRenderer::CreatePoolAndSets()
{
    
    VkDescriptorPoolSize poolSize[4] = {};
    poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize[0].descriptorCount = 2;
    poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize[1].descriptorCount = materials.size();
    poolSize[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSize[2].descriptorCount = 1;
    poolSize[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSize[3].descriptorCount = 2;

    VkDescriptorPoolCreateInfo poolDesc = {};
    poolDesc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolDesc.maxSets = 2;
    poolDesc.poolSizeCount = 4;
    poolDesc.pPoolSizes = poolSize;
    vkCreateDescriptorPool(vulkanBase->device, &poolDesc, nullptr, &descPool);

    VkDescriptorSetLayout layouts[2] = { descSetLayout, computeSetLayout};
    VkDescriptorSet sets[2];
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descPool;
    allocInfo.descriptorSetCount = 2;
    allocInfo.pSetLayouts = layouts;
    vkAllocateDescriptorSets(vulkanBase->device, &allocInfo, sets);
    descSet = sets[0];
    computeDescSet = sets[1];

    /*---- Graphics resources ----*/
    VkDescriptorBufferInfo globalUboBufferInfo = {};
    globalUboBufferInfo.buffer = uboGlobal;
    globalUboBufferInfo.offset = 0;
    globalUboBufferInfo.range = uboGlobalProps.size;

    VkDescriptorBufferInfo perObjBufferInfo = {};
    perObjBufferInfo.buffer = uboPerObj;
    perObjBufferInfo.offset = 0;
    perObjBufferInfo.range = uboPerObjProps.size;

    vector<VkDescriptorImageInfo> imgInfos;
    for (int i = 0; i < materials.size(); i++)
    {
        imgInfos.push_back({});
        imgInfos[i].imageView = materials[i].texView;
        imgInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imgInfos[i].sampler = sampler;
    }

    /*---- Compute resources ----*/

    VkDescriptorImageInfo computeImgInfo[2];
    computeImgInfo[0].imageView = vulkanBase->renderTexture.texView;
    computeImgInfo[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    computeImgInfo[0].sampler = nullptr;

    computeImgInfo[1].imageView = vulkanBase->computeTexture.texView;
    computeImgInfo[1].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    computeImgInfo[1].sampler = nullptr;

    VkDescriptorBufferInfo computeDataInfo = {};
    computeDataInfo.buffer = uboCompute;
    computeDataInfo.offset = 0;
    computeDataInfo.range = uboComputeProps.size;

    VkWriteDescriptorSet descriptorWrite[6] = {};
    descriptorWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[0].dstSet = descSet;
    descriptorWrite[0].dstBinding = 0;
    descriptorWrite[0].dstArrayElement = 0;
    descriptorWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite[0].descriptorCount = 1;
    descriptorWrite[0].pBufferInfo = &globalUboBufferInfo;

    descriptorWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[1].dstSet = descSet;
    descriptorWrite[1].dstBinding = 1;
    descriptorWrite[1].dstArrayElement = 0;
    descriptorWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite[1].descriptorCount = imgInfos.size();
    descriptorWrite[1].pImageInfo = imgInfos.data();

    descriptorWrite[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;      
    descriptorWrite[2].dstSet = descSet;
    descriptorWrite[2].dstBinding = 2;
    descriptorWrite[2].dstArrayElement = 0;
    descriptorWrite[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorWrite[2].descriptorCount = 1;
    descriptorWrite[2].pBufferInfo = &perObjBufferInfo;

    /*---- Compute set ----*/
    descriptorWrite[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[3].dstSet = computeDescSet;
    descriptorWrite[3].dstBinding = 0;
    descriptorWrite[3].dstArrayElement = 0;
    descriptorWrite[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorWrite[3].descriptorCount = 1;
    descriptorWrite[3].pImageInfo = &computeImgInfo[0];

    descriptorWrite[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[4].dstSet = computeDescSet;
    descriptorWrite[4].dstBinding = 1;
    descriptorWrite[4].dstArrayElement = 0;
    descriptorWrite[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorWrite[4].descriptorCount = 1;
    descriptorWrite[4].pImageInfo = &computeImgInfo[1];

    descriptorWrite[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[5].dstSet = computeDescSet;
    descriptorWrite[5].dstBinding = 2;
    descriptorWrite[5].dstArrayElement = 0;
    descriptorWrite[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite[5].descriptorCount = 1;
    descriptorWrite[5].pImageInfo = &computeImgInfo[1];

    vkUpdateDescriptorSets(vulkanBase->device, 5, descriptorWrite, 0, nullptr);
}

void VulkanRenderer::CreateSampler()
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(vulkanBase->physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
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

void VulkanRenderer::CreateComputeLayout()
{
    VkDescriptorSetLayoutBinding bindings[3] = {};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo setInfo = {};
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setInfo.bindingCount = 3;
    setInfo.pBindings = bindings;

    vkCreateDescriptorSetLayout(vulkanBase->device, &setInfo, nullptr, &computeSetLayout);

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &computeSetLayout;

    vkCreatePipelineLayout(vulkanBase->device, &layoutInfo, nullptr, &computePipelineLayout);

}

void VulkanRenderer::CreateComputePipeline()
{
    VkShaderModule computeModule;
    shaderc_compile_options_t options = shaderc_compile_options_initialize();
    shaderc_compiler_t compiler = shaderc_compiler_initialize();

    computeModule = compileShader(vulkanBase->device, nullptr, "shader_compute.comp", "main", shaderc_compute_shader, compiler, options);

    shaderc_compiler_release(compiler);
    shaderc_compile_options_release(options);

    VkPipelineShaderStageCreateInfo shaderInfo = {};
    shaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderInfo.module = computeModule;
    shaderInfo.pName = "main";
    shaderInfo.pSpecializationInfo = NULL;
    

    VkComputePipelineCreateInfo computeInfo = {};
    computeInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computeInfo.layout = computePipelineLayout;
    computeInfo.stage = shaderInfo;
    computeInfo.basePipelineHandle = VK_NULL_HANDLE;
    computeInfo.basePipelineIndex = -1;
    
    vkCreateComputePipelines(vulkanBase->device, VK_NULL_HANDLE, 1, &computeInfo, nullptr, &computePipeline);

    vkDestroyShaderModule(vulkanBase->device, computeModule, nullptr);

}

vector<ImageFile*> VulkanRenderer::GenerateTextureArrayCache(aiMaterial** materialArray, uint32_t materialCount, 
                        const std::string& sceneRootPath, const std::string& cachePath)
{
    ImageFile::InitModule();

    vector<ImageFile*> textureFiles;
    textureFiles.resize(materialCount);
    uint32_t arrayPtr = 0;

    constexpr uint32_t pathBufferSize = 400;
    wchar_t path[pathBufferSize];
    uint32_t pathBasePtr = 0;
    while (sceneRootPath[pathBasePtr] != '\0')
    {
        path[pathBasePtr] = sceneRootPath[pathBasePtr];
        pathBasePtr++;
    }
    while (sceneRootPath[pathBasePtr - 1] != '\\') { pathBasePtr--; }
    
    for (uint32_t i = 0; i < materialCount; i++)
    {
        aiMaterial* processedMaterial = materialArray[i];
        aiString str;
        aiReturn ret = processedMaterial->GetTexture(aiTextureType_BASE_COLOR, 0, &str);
        if (ret != aiReturn_SUCCESS)
        {
            continue;
        }

        uint32_t j = 0;
        const char* currentLetter = str.C_Str();
        while (*currentLetter  != '\0' && j + pathBasePtr < pathBufferSize - 1)
        {
            if (*currentLetter == '/')
            {
                if (j + pathBasePtr + 2>= pathBufferSize - 1)
                {
                    OutputDebugStringW(L"Requested path is too long, increase buffer size for texture loading\n");
                    exit(-1);
                }
                path[pathBasePtr + j] = '\\';
                j += 1;
            }
            else
            {
                path[pathBasePtr + j] = *currentLetter;
                j++;
            }
            currentLetter++;
        }
        path[pathBasePtr + j] = '\0';
        textureFiles[arrayPtr] =  new ImageFile(path);
        arrayPtr++;
        
    }

    bool allFinished = false;
    while (!allFinished)
    {
        allFinished = true;
        for (int i = 0; i < arrayPtr; i++)
        {
            if (!textureFiles[i]->loadingIsFinished)
            {
                allFinished = false;
                continue;
            }
        }
    }

    ImageFile::CloseModule();

    fstream cacheFile(cachePath.c_str(), ios::out | ios::binary);
    if (!cacheFile.is_open())
    {
        OutputDebugString(L"Could not create texture array file");
        exit(-1);
    }

    cacheFile.write((char*)  &arrayPtr, sizeof(uint32_t));
    for (uint32_t i = 0; i < arrayPtr; i++)
    {
        uint32_t width = textureFiles[i]->width;
        uint32_t height = textureFiles[i]->height;
        uint32_t format = IMAGE_FORMAT_R8G8B8A8;

        cacheFile.write((char*)&width, sizeof(uint32_t));
        cacheFile.write((char*)&height, sizeof(uint32_t));
        cacheFile.write((char*)&format, sizeof(uint32_t));
    }

    for (uint32_t i = 0; i < arrayPtr; i++)
    {
        cacheFile.write((char*)textureFiles[i]->FileBuff, textureFiles[i]->width * textureFiles[i]->height * sizeof(int));
    }

    cacheFile.close();
    return textureFiles;
}

void VulkanRenderer::UploadImages(const TextureArray& textureArray)
{
    // assert that every image has the same dims
    size_t stagingBufferSize = 0;
    
    for (int i = 0; i < textureArray.images.size(); i++)
    {
        if (textureArray.images[i].format != textureArray.images[0].format)
        {
            OutputDebugString(L"Images with different formats are not supported\n");
            exit(-1);
        }
        Texture tex = create2DTexture(vulkanBase->device, vulkanBase->physicalDevice, 
                   textureArray.images[i].width, textureArray.images[i].height, TEXTURE_FORMAT);
        materials.push_back(tex);
    }

    VkBuffer stagingBuffer;
    void* stagingPtr;
    VkDeviceMemory stagingBufferMem;

    VkBufferCreateInfo buffInfo = {};
    buffInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffInfo.size = textureArray.imageDataSize;
    buffInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(vulkanBase->device, &buffInfo, nullptr, &stagingBuffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vulkanBase->device, stagingBuffer, &memRequirements);
    stagingBufferMem = allocateBuffer(vulkanBase->device, vulkanBase->physicalDevice, memRequirements, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    vkBindBufferMemory(vulkanBase->device, stagingBuffer, stagingBufferMem, 0);

    vkMapMemory(vulkanBase->device, stagingBufferMem, 0, memRequirements.size, 0, &stagingPtr);
    memcpy(stagingPtr, textureArray.dataBuffer, buffInfo.size);
    vkUnmapMemory(vulkanBase->device, stagingBufferMem);

    vector<VkImageMemoryBarrier> copyBarriers;
    vector<VkBufferImageCopy> imgCopies;
    vector<VkImageMemoryBarrier> layoutBarriers;

    for (int i = 0; i < textureArray.images.size(); i++)
    {
        copyBarriers.push_back({});
        copyBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        copyBarriers[i].srcAccessMask = 0;
        copyBarriers[i].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
        copyBarriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        copyBarriers[i].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        copyBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copyBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copyBarriers[i].image = materials[i].texImage;
        copyBarriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyBarriers[i].subresourceRange.baseMipLevel = 0;
        copyBarriers[i].subresourceRange.levelCount = 1;
        copyBarriers[i].subresourceRange.baseArrayLayer = 0;
        copyBarriers[i].subresourceRange.layerCount = 1;

        imgCopies.push_back({});
        imgCopies[i].bufferOffset = textureArray.images[i].bufferOffset;
        imgCopies[i].bufferRowLength = 0;
        imgCopies[i].bufferImageHeight = 0;
        imgCopies[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imgCopies[i].imageSubresource.mipLevel = 0;
        imgCopies[i].imageSubresource.baseArrayLayer = 0;
        imgCopies[i].imageSubresource.layerCount = 1;
        imgCopies[i].imageOffset = { 0, 0, 0 };
        imgCopies[i].imageExtent = {
            (unsigned int)textureArray.images[i].width,
            (unsigned int)textureArray.images[i].height,
            1
        };

        layoutBarriers.push_back({});
        layoutBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        layoutBarriers[i].srcAccessMask = 0;
        layoutBarriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        layoutBarriers[i].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        layoutBarriers[i].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        layoutBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        layoutBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        layoutBarriers[i].image = materials[i].texImage;
        layoutBarriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        layoutBarriers[i].subresourceRange.baseMipLevel = 0;
        layoutBarriers[i].subresourceRange.levelCount = 1;
        layoutBarriers[i].subresourceRange.baseArrayLayer = 0;
        layoutBarriers[i].subresourceRange.layerCount = 1;

    }

    // copy buffer to memory and transition all resources
    VkCommandBufferBeginInfo cmdBuffInfo = {};
    cmdBuffInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBuffInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkResetCommandBuffer(vulkanBase->cmdBuffer, 0);
    vkBeginCommandBuffer(vulkanBase->cmdBuffer, &cmdBuffInfo);

    vkCmdPipelineBarrier(vulkanBase->cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, copyBarriers.size(), copyBarriers.data());

    for (int i = 0; i < materials.size(); i++)
    {
        vkCmdCopyBufferToImage(vulkanBase->cmdBuffer, stagingBuffer, materials[i].texImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopies[i]);
    }
    vkCmdPipelineBarrier(vulkanBase->cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, layoutBarriers.size(), layoutBarriers.data());

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



void VulkanRenderer::DrawItem(size_t idx)
{
    Object* item = &renderableItems[idx];
    uint32_t dynamicOffsetCount[1] = { item->uboOffset };
    vkCmdBindDescriptorSets(vulkanBase->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descSet, 1, dynamicOffsetCount);
    for (size_t i = 0; i < item->meshIdx.size(); i++)
    {
        uint32_t currentMesh = item->meshIdx[i];
        uint32_t materialIndex = sceneGeometry.materialIndex[currentMesh];
        int constants[4] = {materialTextureIdx[materialIndex], 0, 0, 0};
        if (materialTextureIdx[materialIndex] == 0 && materialIndex != 0)
        {
            continue;
        }
        vkCmdPushConstants(vulkanBase->cmdBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4 * sizeof(int), constants);
        vkCmdDrawIndexed(vulkanBase->cmdBuffer, sceneGeometry.indexCount[currentMesh], 1, sceneGeometry.ibOffset[currentMesh], sceneGeometry.vbOffset[currentMesh], 0);
    }

}

std::vector<char> VulkanRenderer::readFile(const std::string& filename)
{
    ifstream file(filename, ios::ate | ios::binary);

    if (!file.is_open()) 
    {
        wstring msg(filename.begin(), filename.end());
        msg = L"Failed to open file " + msg;
        MessageBox(NULL, msg.c_str(), NULL, MB_OK);
        exit(-1);
    }

    size_t fileSize = (size_t)file.tellg();
    vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
    
}
