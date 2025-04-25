#pragma once

#include "VulkanBase.hpp"
#include <string>

class VulkanRenderer
{
public:
	VulkanRenderer(HINSTANCE hinstance, HWND hwnd);
	void Render();
	void updateRotation();
	~VulkanRenderer();
private:
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUbo();
	void CreateGraphicsPipeline();
	void CreatePipelineLayout();
	void CreatePoolAndSets();
	static std::vector<char> readFile(const std::string& filename);
private:
	VkBuffer vertexBuffer;
	VkDeviceMemory vbDevMem;
	VkBuffer indexBuffer;
	VkDeviceMemory ibDevMem;
	VkBuffer stagingBuffer;
	VkDeviceMemory stDevMem;
	VkBuffer uboBuffer;
	VkBuffer uboBuffer2;
	VkDeviceMemory uboDevMem;
	void* uboData;
	float angle = 0.0f;

	VkDescriptorPool descPool;
	VkDescriptorSet descSet;
	VkDescriptorSetLayout descSetLayout;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
	VulkanBase* vulkanBase;
};
