#pragma once

#include "VulkanBase.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "VulkanOps.hpp"
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "ImageFile.h"
#include "TextureArray.h"

#define SQUARE_COUNT_X 5
#define SQUARE_COUNT_Z 5
#define SQUARE_COUNT_Y 5
#define SQUARE_COUNT  (SQUARE_COUNT_X * SQUARE_COUNT_Z * SQUARE_COUNT_Y)

struct MeshCollection
{
	VkBuffer vertexBuffer;
	VkDeviceMemory vbDevMem;
	VkBuffer indexBuffer;
	VkDeviceMemory ibDevMem;
	std::vector<uint32_t> vbOffset;
	std::vector<uint32_t> ibOffset;
	std::vector<uint32_t> indexCount;
	std::vector<uint32_t> materialIndex;
};

struct GlobalUbo
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec4 lightPos; // w component is 
	glm::vec4 lightCol; // w component is 
	//matrix proj;
	//char alignment[256 - (224 - 4 * 4) ];
};

struct PerObjUbo
{
	glm::mat4 model;
	glm::ivec4 index;
};


struct Object
{
	std::vector<size_t> meshIdx;
	uint32_t uboOffset;
	std::string name;
	PerObjUbo transformation;
};


class VulkanRenderer
{
public:
	VulkanRenderer(HINSTANCE hinstance, HWND hwnd, const std::string& path);
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
	std::vector<ImageFile*> GenerateTextureArrayCache(aiMaterial** materialArray, uint32_t materialCount, 
												const std::string& sceneRootPath, const std::string& cachePath);
	void UploadImages(const TextureArray& textureArray);
	void DrawItem(size_t idx);
	void loadScene(const std::string& path);
	void parseObjectTree(aiNode* node, const glm::mat4x4& transform);
	static std::vector<char> readFile(const std::string& filename);
private:
	MeshCollection geometry;
	MeshCollection sceneGeometry;
	std::vector<uint32_t> materialTextureIdx;
	VkBuffer uboGlobal;
	VkBuffer uboPerObj;
	BufferMemoryProperties uboGlobalProps;
	BufferMemoryProperties uboPerObjProps;
	std::vector<Object> renderableItems;

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
	std::vector<Texture> materials;
	VkSampler sampler;
};
