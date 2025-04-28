#include "VulkanBase.hpp"
#include <vector>
#include "VulkanBaseInternal.hpp"
#pragma comment(lib,"C:\\VulkanSDK\\1.4.304.1\\Lib\\vulkan-1.lib")
using namespace std;
const static char* instExt[] = {
					 VK_KHR_SURFACE_EXTENSION_NAME,
					 VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
					 VK_KHR_WIN32_SURFACE_EXTENSION_NAME};


const static char* devExt[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

const static char* vaLayers[] = { "VK_LAYER_KHRONOS_validation" };

void validateVkResult(VkResult result)
{
	if (result == VK_SUCCESS)
	{
		return;
	}
	MessageBox(NULL, L"vkResult is error", NULL, MB_OK);
	exit(-1);
}

void exitOnError(const wchar_t* errMsg)
{
	MessageBox(NULL, errMsg, NULL, MB_OK);
	exit(-1);
}

VkBool32 vbDebugVal(
	VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		OutputDebugStringA("validation layer: ");
		OutputDebugStringA(pCallbackData->pMessage);
		OutputDebugStringA("\n");
	}
	return VK_FALSE;
}

VkInstance createInstance()
{
	uint32_t propCount;
	CHECK_VK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &propCount, nullptr));
	vector<VkExtensionProperties> instExtSupported(propCount);
	CHECK_VK_RESULT(vkEnumerateInstanceExtensionProperties( nullptr, &propCount, instExtSupported.data()));
	if (!checkSupportForExt(instExt, sizeof(instExt) / sizeof(const char*), instExtSupported.data(), instExtSupported.size()))
	{
		OutputDebugString(L"\nUnsupported extension found! \n");
		exit(-1);
	}

	VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
	debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugInfo.pfnUserCallback = vbDebugVal;

	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;
	appInfo.pNext = NULL;


	VkInstanceCreateInfo instanceInfo = {};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugInfo;;
	instanceInfo.enabledExtensionCount = sizeof(instExt) / sizeof(const char*);
	instanceInfo.ppEnabledExtensionNames = instExt;
	instanceInfo.enabledLayerCount = sizeof(vaLayers) / sizeof(const char*);
	instanceInfo.ppEnabledLayerNames = vaLayers;

	VkInstance instance = VK_NULL_HANDLE;
	CHECK_VKRESULT(vkCreateInstance(&instanceInfo, nullptr, &instance));
	return instance;
}

VulkanBase* createVulkanBase(HINSTANCE hinstance, HWND hwnd)
{
	void* mem = _aligned_malloc(sizeof(VulkanBase), 64);
	VulkanBase* vulkanBase = new(mem) VulkanBase;

	vulkanBase->instance = createInstance();

#ifdef _DEBUG
	vulkanBase->debug = setupDebug(vulkanBase->instance);
#endif 
	vulkanBase->physicalDevice = pickPhysicalDevice(vulkanBase->instance);

	vulkanBase->surface = createSurface(vulkanBase->instance, hinstance, hwnd);
	vulkanBase->queueFamilies = createQueueIndecies(vulkanBase->instance, vulkanBase->physicalDevice, vulkanBase->surface);
	vulkanBase->device = createLogicalDevice(vulkanBase->physicalDevice, vulkanBase->queueFamilies);
	vkGetDeviceQueue(vulkanBase->device, vulkanBase->queueFamilies.grahicsIdx, 0, &vulkanBase->graphicsQueue);
	vkGetDeviceQueue(vulkanBase->device, vulkanBase->queueFamilies.presentationIdx, 0, &vulkanBase->presentationQueue);
	vulkanBase->swapchainInfo = querySwapChainSupport(vulkanBase->physicalDevice, vulkanBase->surface);
	vulkanBase->swapchain = createSwapchain(vulkanBase->device, vulkanBase->surface, vulkanBase->swapchainInfo, 
											vulkanBase->queueFamilies, &vulkanBase->swapchainFormat);
	vulkanBase->swapchainImages = createSwapchainImages(vulkanBase->device, vulkanBase->swapchain);
	vulkanBase->swapchainImageViews = createSwapchainImageViews(vulkanBase->device, vulkanBase->swapchainImages, 
																vulkanBase->swapchainInfo, vulkanBase->swapchainFormat);
	vulkanBase->cmdPool = createCommandPool(vulkanBase->device, vulkanBase->queueFamilies.grahicsIdx);
	vulkanBase->cmdBuffer = createCommandBuffers(vulkanBase->device, vulkanBase->cmdPool, 1)[0];
	vulkanBase->renderPass = createRenderPass(vulkanBase->device, vulkanBase->swapchainFormat);
	DepthBufferBundle bundle = createDepthBuffer(vulkanBase->device, vulkanBase->physicalDevice, vulkanBase->swapchainInfo);
	vulkanBase->depthImage = bundle.depthImage;
	vulkanBase->depthImageMemory = bundle.depthImageMemory;
	vulkanBase->depthImageView = bundle.depthImageView;
	vulkanBase->swapchainFramebuffers = createFramebuffers(vulkanBase->device, vulkanBase->renderPass, vulkanBase->depthImageView,
															vulkanBase->swapchainImageViews, vulkanBase->swapchainInfo);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	CHECK_VK_RESULT(vkCreateSemaphore(vulkanBase->device, &semaphoreInfo, nullptr, &vulkanBase->imgReady));
	CHECK_VK_RESULT(vkCreateSemaphore(vulkanBase->device, &semaphoreInfo, nullptr, &vulkanBase->renderingFinished));

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	CHECK_VK_RESULT(vkCreateFence(vulkanBase->device, &fenceInfo, nullptr, &vulkanBase->gfxQueueFinished));

	return vulkanBase;
}

VkPhysicalDevice pickPhysicalDevice(VkInstance instance)
{
	uint32_t count;
	CHECK_VK_RESULT(vkEnumeratePhysicalDevices(instance, &count, nullptr));
	vector<VkPhysicalDevice> physicalDevices(count, VK_NULL_HANDLE);
	CHECK_VK_RESULT(vkEnumeratePhysicalDevices(instance, &count, physicalDevices.data()));
	
	for (VkPhysicalDevice dev : physicalDevices)
	{
		VkPhysicalDeviceProperties props = {};
		VkPhysicalDeviceFeatures features = {};
		vkGetPhysicalDeviceProperties(dev, &props);
		vkGetPhysicalDeviceFeatures(dev, &features);

		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && features.shaderFloat64)
		{
			return dev;
		}
	}
	return VK_NULL_HANDLE;
}

VkDebugUtilsMessengerEXT setupDebug(VkInstance instance)
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = vbDebugVal;
	createInfo.pUserData = nullptr; // Optional
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	
	VkDebugUtilsMessengerEXT debug;
	CHECK_VK_RESULT(func(instance, &createInfo, nullptr, &debug));
	return debug;
}

VkSurfaceKHR createSurface(VkInstance instance, HINSTANCE hinstance, HWND hwnd)
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkWin32SurfaceCreateInfoKHR surfInfo = {};
	surfInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfInfo.hinstance = hinstance;
	surfInfo.hwnd = hwnd;

	CHECK_VK_RESULT(vkCreateWin32SurfaceKHR(instance, &surfInfo, nullptr, &surface));
	return surface;
}

QueuesIdx createQueueIndecies(VkInstance instance, VkPhysicalDevice dev, VkSurfaceKHR surface)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());

	QueuesIdx queues = { -1, -1 };
	for (int64_t i = 0; i < queueFamilies.size(); i++)
	{
		if (queues.grahicsIdx == -1 && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
			(queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)))
		{
			queues.grahicsIdx = i;
		}

		VkBool32 surfaceSupport;
		CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &surfaceSupport) );
		if (queues.presentationIdx == -1 && surfaceSupport)
		{
			queues.presentationIdx = i;
		}

		if (queues.presentationIdx != -1 && queues.grahicsIdx != -1)
		{
			break;
		}
	}

	if (queues.presentationIdx == -1 || queues.grahicsIdx == -1)
	{
		OutputDebugString(L"Required queue is not supported in the system \n");
		exit(-1);
	}
	return queues;
}

VkDevice createLogicalDevice(VkPhysicalDevice physicalDev, QueuesIdx queuesIdx)
{
	uint32_t propCount;
	CHECK_VK_RESULT(vkEnumerateDeviceExtensionProperties(physicalDev, nullptr, &propCount, nullptr));
	vector<VkExtensionProperties> devExtSupported(propCount);
	CHECK_VK_RESULT(vkEnumerateDeviceExtensionProperties(physicalDev, nullptr, &propCount, devExtSupported.data()));
	
	if (!checkSupportForExt(devExt, sizeof(devExt) / sizeof(const char*), devExtSupported.data(), devExtSupported.size()))
	{
		OutputDebugString(L"\nUnsupported extension found! \n");
		exit(-1);
	}

	VkDevice device;
	float prioGfx[] = { 1.0f, 1.0f }, prioPres[] = { 1.0f };
	uint32_t familyCount = 1;
	VkDeviceQueueCreateInfo queueInfos[2] = {};
	queueInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfos[0].queueFamilyIndex = queuesIdx.grahicsIdx;
	queueInfos[0].queueCount = 1;
	queueInfos[0].pQueuePriorities = prioGfx;
	if (queuesIdx.grahicsIdx != queuesIdx.presentationIdx)
	{
		familyCount = 2;
		queueInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfos[1].queueFamilyIndex = queuesIdx.presentationIdx;
		queueInfos[1].queueCount = 1;
		queueInfos[1].pQueuePriorities = prioPres;
	}

	VkPhysicalDeviceFeatures features = {};
	
	VkDeviceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	info.pQueueCreateInfos = queueInfos;
	info.queueCreateInfoCount = familyCount;
	info.enabledLayerCount = 1;
	info.ppEnabledLayerNames = vaLayers;
	info.enabledExtensionCount = 1;
	info.ppEnabledExtensionNames = devExt;
	info.pEnabledFeatures = &features;
	CHECK_VK_RESULT(vkCreateDevice(physicalDev, &info, nullptr, &device));
	return device;
	
}

SwapchainInfo querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	SwapchainInfo info;
	CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &info.capabilities));
	
	uint32_t formatCount;
	CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr));
	if (formatCount != 0) 
	{
		info.formats.resize(formatCount);
		CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, info.formats.data()));
	}
	
	uint32_t presentModeCount;
	CHECK_VK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr));
	if (presentModeCount != 0) 
	{
		info.presentModes.resize(presentModeCount);
		CHECK_VK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, info.presentModes.data()));
	}

	return info;
}

bool checkSupportForExt(const char** requiredExtensions, uint32_t requiredExtensionsNum,
						const VkExtensionProperties* supportedExtensions, uint32_t supportedExtensionsNum)
{
	uint32_t requiredExtensionsCount = 0;
	uint8_t* requiredExtFlags = new uint8_t[requiredExtensionsNum];
	memset(requiredExtFlags, 0, sizeof(uint8_t) * requiredExtensionsNum);

	for (uint32_t i = 0; i < supportedExtensionsNum; i++)
	{
		const char* extName = supportedExtensions[i].extensionName;
		for (uint32_t j = 0; j < requiredExtensionsNum; j++)
		{
			if (requiredExtFlags[j] != 1 && strcmp(extName, requiredExtensions[j]) == 0)
			{
				requiredExtFlags[j] = 1;
				requiredExtensionsCount++;
				break;
			}
		}

		if (requiredExtensionsCount == requiredExtensionsNum)
		{
			delete[] requiredExtFlags;
			return true;
		}
	}
	delete[] requiredExtFlags;
	return false;
}

std::vector<VkImage> createSwapchainImages(VkDevice device, VkSwapchainKHR swapchain)
{
	std::vector<VkImage> images;
	uint32_t imageCount;
	CHECK_VK_RESULT(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));
	images.resize(imageCount);
	CHECK_VK_RESULT(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data()));

	return images;
}

std::vector<VkImageView> createSwapchainImageViews(VkDevice device, const std::vector<VkImage>& images, 
													const SwapchainInfo& swcInfo, const VkFormat& imgFormat)
{

	std::vector<VkImageView> views(images.size());
	for (size_t i = 0; i < images.size(); i++)
	{
		VkImageViewCreateInfo imgInfo = {};
		imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imgInfo.image = images[i];
		imgInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imgInfo.format = imgFormat;
		imgInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imgInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imgInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imgInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imgInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgInfo.subresourceRange.baseMipLevel = 0;
		imgInfo.subresourceRange.levelCount = 1;
		imgInfo.subresourceRange.baseArrayLayer = 0;
		imgInfo.subresourceRange.layerCount = 1;

		CHECK_VK_RESULT( vkCreateImageView(device, &imgInfo, nullptr, &views[i]));
	}

	return views;
}

VkCommandPool createCommandPool(VkDevice device, uint32_t queueIdx)
{
	VkCommandPool cmdPool;
	VkCommandPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	info.queueFamilyIndex = queueIdx;

	CHECK_VK_RESULT(vkCreateCommandPool(device, &info, nullptr, &cmdPool));
	return cmdPool;
}

std::vector<VkCommandBuffer> createCommandBuffers(VkDevice device, VkCommandPool cmdPool, uint32_t bufferCount)
{
	std::vector<VkCommandBuffer> cmdBuffers(bufferCount);

	VkCommandBufferAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.commandBufferCount = bufferCount;
	info.commandPool = cmdPool;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	CHECK_VK_RESULT(vkAllocateCommandBuffers(device, &info, cmdBuffers.data()));
	return cmdBuffers;
}

VkRenderPass createRenderPass(VkDevice device, VkFormat imgFormat)
{
	VkRenderPass renderPass;
	VkAttachmentDescription attachmentDesc[2] = {};
	attachmentDesc[0].format = imgFormat;
	attachmentDesc[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDesc[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDesc[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDesc[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDesc[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDesc[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDesc[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachmentDesc[1].format = VK_FORMAT_D32_SFLOAT_S8_UINT;
	attachmentDesc[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDesc[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDesc[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDesc[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDesc[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDesc[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDesc[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = 2;
	info.pAttachments = attachmentDesc;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dependency;

	CHECK_VK_RESULT(vkCreateRenderPass(device, &info, nullptr, &renderPass));

	return renderPass;
}

std::vector<VkFramebuffer> createFramebuffers(VkDevice device, VkRenderPass renderPass, VkImageView depthView,
												const std::vector<VkImageView> imgViews, const SwapchainInfo& swcInfo)
{
	vector<VkFramebuffer> framebuffers(imgViews.size());
	for (size_t i = 0; i < imgViews.size(); i++)
	{
		VkImageView views[2] = { imgViews[i], depthView };

		VkFramebufferCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.renderPass = renderPass;
		info.attachmentCount = 2;
		info.pAttachments = views;
		info.width = swcInfo.capabilities.currentExtent.width;
		info.height = swcInfo.capabilities.currentExtent.height;
		info.layers = 1;
		CHECK_VK_RESULT(vkCreateFramebuffer(device, &info, nullptr, &framebuffers[i]));
	}
	return framebuffers;
}

DepthBufferBundle createDepthBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const SwapchainInfo& swcInfo)
{
	DepthBufferBundle bundleOut = {};


	VkFormatProperties props;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, VK_FORMAT_D32_SFLOAT_S8_UINT, &props);
	if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == 0)
	{
		exitOnError(L"Unsupported VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT for optimal features\n");
	}

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = swcInfo.capabilities.currentExtent.width;
	imageInfo.extent.height = swcInfo.capabilities.currentExtent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	CHECK_VK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &bundleOut.depthImage));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, bundleOut.depthImage, &memRequirements);

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	uint32_t i;
	for (i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((memRequirements.memoryTypeBits & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		{
			break;
		}
	}

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = i;
	CHECK_VK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &bundleOut.depthImageMemory));
	CHECK_VK_RESULT(vkBindImageMemory(device, bundleOut.depthImage, bundleOut.depthImageMemory, 0));

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = bundleOut.depthImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	CHECK_VK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &bundleOut.depthImageView));
	return bundleOut;
}

VkSwapchainKHR createSwapchain(VkDevice device, VkSurfaceKHR surface, const SwapchainInfo& swapchainInfo, 
								const QueuesIdx& queuesIdx, VkFormat* chosenFormat)
{
	size_t i;
	for (i = 0; i < swapchainInfo.formats.size(); i++)
	{
		if(swapchainInfo.formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
		   swapchainInfo.formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			break;
		}
	}
	if (i == swapchainInfo.formats.size())
	{
		OutputDebugString(L"Unsupported VK_FORMAT_B8G8R8A8_SRGB! \n");
		exit(-1);
	}

	if (chosenFormat)
	{
		*chosenFormat = swapchainInfo.formats[i].format;
	}
	uint32_t imgCount = swapchainInfo.capabilities.minImageCount + 1 <= swapchainInfo.capabilities.maxImageCount ?
						swapchainInfo.capabilities.minImageCount + 1 : swapchainInfo.capabilities.minImageCount;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkSwapchainCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = surface;
	info.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.imageArrayLayers = 1;
	info.imageExtent = swapchainInfo.capabilities.currentExtent;
	info.minImageCount = imgCount;
	info.preTransform = swapchainInfo.capabilities.currentTransform;
	info.imageFormat = swapchainInfo.formats[i].format;
	info.imageColorSpace = swapchainInfo.formats[i].colorSpace;
	info.clipped = VK_FALSE;
	info.oldSwapchain = VK_NULL_HANDLE;

	if (queuesIdx.grahicsIdx != queuesIdx.presentationIdx) 
	{
		uint32_t queueFamilies[] = { queuesIdx.grahicsIdx, queuesIdx.presentationIdx };
		info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		info.queueFamilyIndexCount = 2;
		info.pQueueFamilyIndices = queueFamilies;
	}
	else 
	{
		info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.queueFamilyIndexCount = 0; // Optional
		info.pQueueFamilyIndices = nullptr; // Optional
	}

	CHECK_VK_RESULT(vkCreateSwapchainKHR(device, &info, nullptr, &swapchain));
	
	return swapchain;
}
