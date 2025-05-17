#pragma once

#include "VulkanBase.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "VulkanOps.hpp"
#include <string>

struct GeometryCollection
{
	VkBuffer vertexBuffer;
	VkDeviceMemory vbDevMem;
	VkBuffer indexBuffer;
	VkDeviceMemory ibDevMem;
	std::vector<uint32_t> vbOffset;
	std::vector<uint32_t> ibOffset;
	std::vector<uint32_t> indexCount;

};

class VulkanRenderer
{
public:
	VulkanRenderer(HINSTANCE hinstance, HWND hwnd);
	void Render();
	void updateRotation();
	void updateCameraLH(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);
	~VulkanRenderer();
private:
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUbo();
	void CreateGraphicsPipeline();
	void CreatePipelineLayout();
	void CreatePoolAndSets();
	void CreateSampler();
	void PrepareTexture();
	static std::vector<char> readFile(const std::string& filename);
private:
	GeometryCollection geometry;
	VkBuffer uboGlobal;
	VkBuffer uboPerObj;
	BufferMemoryProperties uboGlobalProps;
	BufferMemoryProperties uboPerObjProps;

	VkDeviceMemory uboDevMem;
	void* uboData;
	float angle = 0.0f;

	VkDescriptorPool descPool;
	VkDescriptorSet descSet;
	VkDescriptorSetLayout descSetLayout;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
	VulkanBase* vulkanBase;
	Texture tex;
	VkSampler sampler;
};
