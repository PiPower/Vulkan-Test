#pragma once 
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#define CHECK_VKRESULT(result) validateVkResult(result)

VkBool32 vbDebugVal(
	VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData);

struct DepthBufferBundle
{
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
};

void validateVkResult(VkResult result);
VkInstance createInstance();
VkPhysicalDevice pickPhysicalDevice(VkInstance instance);
VkDebugUtilsMessengerEXT setupDebug(VkInstance instance);
VkSurfaceKHR createSurface(VkInstance instance, HINSTANCE hinstance, HWND hwnd);
QueuesIdx createQueueIndecies(VkInstance instance, VkPhysicalDevice dev, VkSurfaceKHR surface);
VkDevice createLogicalDevice(VkPhysicalDevice physicalDev, QueuesIdx queuesIdx);
SwapchainInfo querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
bool checkSupportForExt(const char** requiredExtensions, uint32_t requiredExtensionsNum,
						const VkExtensionProperties* supportedExtensions, uint32_t supportedExtensionsNum);
VkSwapchainKHR createSwapchain(VkDevice device, VkSurfaceKHR surface, const SwapchainInfo& swapchainInfo,
								const QueuesIdx& queuesIdx, VkFormat* chosenFormat = nullptr);
std::vector<VkImage> createSwapchainImages(VkDevice device, VkSwapchainKHR swapchain);
std::vector<VkImageView> createSwapchainImageViews(VkDevice device, const std::vector<VkImage>& images, 
													const SwapchainInfo& swcInfo,const VkFormat& imgFormat);
VkCommandPool createCommandPool(VkDevice device, uint32_t queueIdx);
std::vector<VkCommandBuffer> createCommandBuffers(VkDevice device, VkCommandPool cmdPool, uint32_t bufferCount);
VkRenderPass createRenderPass(VkDevice device, VkFormat imgFormat);
std::vector<VkFramebuffer> createFramebuffers(VkDevice device, VkRenderPass renderPass, 
												const std::vector<VkImageView> imgViews, const SwapchainInfo& swcInfo);
DepthBufferBundle createDepthBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const SwapchainInfo& swcInfo);