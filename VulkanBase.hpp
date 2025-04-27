#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <Windows.h>
#include <vector>

struct QueuesIdx
{
	int64_t grahicsIdx;
	int64_t presentationIdx;
};

struct SwapchainInfo
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct VulkanBase
{
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkSurfaceKHR surface;
	QueuesIdx queueFamilies;
	VkQueue graphicsQueue;
	VkQueue presentationQueue;
	VkDebugUtilsMessengerEXT debug;
	SwapchainInfo swapchainInfo;
	VkFormat swapchainFormat;
	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkFramebuffer> swapchainFramebuffers;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	VkCommandPool cmdPool;
	VkCommandBuffer cmdBuffer;
	VkRenderPass renderPass;
	VkSemaphore imgReady;
	VkSemaphore renderingFinished;
	VkFence gfxQueueFinished;
};

VulkanBase* createVulkanBase(HINSTANCE hinstance, HWND hwnd);