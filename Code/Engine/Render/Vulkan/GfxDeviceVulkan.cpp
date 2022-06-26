
#include "GfxDeviceVulkan.h"

#include <vector>
#include <array>

#include "Apps/Error.h"
#include "Apps/window/WindowInclude.h"
#include "Apps/window/GlfwWindow.h"
#include "Apps/FileSystem.h"
#include "Render/GlslCompiler.h"

GfxDeviceVulkan::GfxDeviceVulkan(BasicWindow* window)
{
	// Extension of interest in this sample (optional)
	AddDeviceExtension(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME, true);

	// Extension dependency requirements (given that instance API version is 1.0.0)
	AddInstanceExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, true);
	AddDeviceExtension(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, true);
	AddDeviceExtension(VK_KHR_MAINTENANCE2_EXTENSION_NAME, true);
	AddDeviceExtension(VK_KHR_MULTIVIEW_EXTENSION_NAME, true);
	AddDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME, true);

	InitInstance();

	GlfwWindow* glWindow = static_cast<GlfwWindow*>(window);
	m_GfxContext.surface = glWindow->CreateSurface(m_GfxContext.instance);
	WindowsProperty& wProperty = window->GetWindowsProperty();
	m_GfxContext.swapchainDimensions.width = wProperty.extent.width;
	m_GfxContext.swapchainDimensions.height = wProperty.extent.height;

	InitDevice();
	InitSwapchain(wProperty.extent.width, wProperty.extent.height);
	InitRenderPass();
	InitPipeline();
	InitFrameBuffers();
}

GfxDeviceVulkan::~GfxDeviceVulkan()
{

}

void GfxDeviceVulkan::Initialized()
{

}

void GfxDeviceVulkan::Update()
{
	uint32_t index;
	auto res = AcquireNextImage(&index);
	if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		//resize
		res = AcquireNextImage(&index);
	}

	if (res != VK_SUCCESS)
	{
		vkQueueWaitIdle(m_GfxContext.queue);
		return;
	}

	Render(index);
	res = PresentImage(index);
}

void GfxDeviceVulkan::BeginFrame()
{
	
}

#if defined(GFX_DEBUG) || defined(VKB_VALIDATION_LAYERS)
/// @brief A debug callback called from Vulkan validation layers.
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT type,
	uint64_t object, size_t location, int32_t message_code,
	const char* layer_prefix, const char* message, void* user_data)
{
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		LOGE("Validation Layer: Error: {}: {}", layer_prefix, message);
	}
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		LOGE("Validation Layer: Warning: {}: {}", layer_prefix, message);
	}
	else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
	{
		LOGI("Validation Layer: Performance warning: {}: {}", layer_prefix, message);
	}
	else
	{
		LOGI("Validation Layer: Information: {}: {}", layer_prefix, message);
	}
	return VK_FALSE;
}
#endif

void GfxDeviceVulkan::InitInstance()
{
	if (volkInitialize())
	{
		throw std::runtime_error("Failed to initialize volk.");
	}

	uint32_t instanceExtensionCount;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));

	std::vector<VkExtensionProperties> instanceExtensions(instanceExtensionCount);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensions.data()));

	std::vector<const char*> activeInstanceExtensions({ VK_KHR_SURFACE_EXTENSION_NAME });

#if defined(GFX_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	activeInstanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	activeInstanceExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
	activeInstanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
	activeInstanceExtensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	activeInstanceExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
	activeInstanceExtensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	activeInstanceExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
	activeInstanceExtensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
#else
#	pragma error Platform not supported
#endif

	//for (auto extension: m_InstanceExtenstions)
	//{
	//	auto extensionName = extension.first;
	//	auto extensionIsEnabled = extension.second;

	//	if (std::find_if(instanceExtensions.begin(), instanceExtensions.end(),
	//		[&extensionName](VkExtensionProperties availableExtension) { return strcmp(availableExtension.extensionName, extensionName) == 0;}) == instanceExtensions.end())
	//	{
	//		// ...
	//	}
	//	else
	//	{
	//		activeInstanceExtensions.push_back(extensionName);
	//	}
	//}

	uint32_t instanceLayerCount;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));

	std::vector<VkLayerProperties> supportedValidationLayer(instanceLayerCount);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&instanceLayerCount, supportedValidationLayer.data()));

	std::vector<const char*> requestedValidationLayers = {};

	VkApplicationInfo app{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	app.pApplicationName = "Test";
	app.pEngineName = "WEngine";
	app.apiVersion = VK_MAKE_VERSION(1, 0, 0);

	VkInstanceCreateInfo instanceInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instanceInfo.pApplicationInfo = &app;
	instanceInfo.enabledExtensionCount = activeInstanceExtensions.size();
	instanceInfo.ppEnabledExtensionNames = activeInstanceExtensions.data();
	instanceInfo.enabledLayerCount = requestedValidationLayers.size();
	instanceInfo.ppEnabledLayerNames = requestedValidationLayers.data();

#if defined(GFX_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	VkDebugReportCallbackCreateInfoEXT debugReportCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
	debugReportCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	debugReportCreateInfo.pfnCallback = DebugCallback;

	instanceInfo.pNext = &debugReportCreateInfo;
#endif

	VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &m_GfxContext.instance));

	volkLoadInstance(m_GfxContext.instance);

#if defined(GFX_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	VK_CHECK(vkCreateDebugReportCallbackEXT(m_GfxContext.instance, &debugReportCreateInfo, nullptr, &m_GfxContext.debugCallback));
#endif
}

void GfxDeviceVulkan::InitDevice()
{
	uint32_t gpuCount = 0;
	VK_CHECK(vkEnumeratePhysicalDevices(m_GfxContext.instance, &gpuCount, nullptr));
	if (gpuCount < 1)
	{
		throw std::runtime_error("No physical device found.");
	}

	std::vector<VkPhysicalDevice> gpus(gpuCount);
	VK_CHECK(vkEnumeratePhysicalDevices(m_GfxContext.instance, &gpuCount, gpus.data()));

	for (size_t i = 0; i < gpuCount && (m_GfxContext.graphicsQueueIndex < 0); i++)
	{
		m_GfxContext.vkPhysicalDevice = gpus[i];
		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(m_GfxContext.vkPhysicalDevice, &queueFamilyCount, nullptr);

		if (queueFamilyCount < 1)
		{
			throw std::runtime_error("No queue family found.");
		}

		std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_GfxContext.vkPhysicalDevice, &queueFamilyCount, queueFamilyProperties.data());
		for (uint32_t j = 0; j < queueFamilyCount; j++)
		{
			VkBool32 supportsPresent;
			vkGetPhysicalDeviceSurfaceSupportKHR(m_GfxContext.vkPhysicalDevice, j, m_GfxContext.surface, &supportsPresent);

			if ((queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) && supportsPresent)
			{
				m_GfxContext.graphicsQueueIndex = j;
				break;
			}
		}
	}

	if (m_GfxContext.graphicsQueueIndex < 0)
	{
		// log

	}

	uint32_t deviceExtensionCount;
	VK_CHECK(vkEnumerateDeviceExtensionProperties(m_GfxContext.vkPhysicalDevice, nullptr, &deviceExtensionCount, nullptr));
	std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
	VK_CHECK(vkEnumerateDeviceExtensionProperties(m_GfxContext.vkPhysicalDevice, nullptr, &deviceExtensionCount, deviceExtensions.data()));

	float queuePriority = 1.0f;

	std::vector<const char*> requiredDeviceExtensions;
	for (auto& extension: m_DeviceExtensions)
	{
		requiredDeviceExtensions.emplace_back(extension.first);
	}

	VkDeviceQueueCreateInfo queueInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	queueInfo.queueFamilyIndex = m_GfxContext.graphicsQueueIndex;
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = &queuePriority;

	VkDeviceCreateInfo deviceInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pQueueCreateInfos = &queueInfo;
	deviceInfo.enabledExtensionCount = requiredDeviceExtensions.size();
	deviceInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

	VK_CHECK(vkCreateDevice(m_GfxContext.vkPhysicalDevice, &deviceInfo, nullptr, &m_GfxContext.device));
	volkLoadDevice(m_GfxContext.device);

	vkGetDeviceQueue(m_GfxContext.device, m_GfxContext.graphicsQueueIndex, 0, &m_GfxContext.queue);
}

void GfxDeviceVulkan::InitSwapchain(uint32_t width, uint32_t height)
{
	VkSurfaceCapabilitiesKHR surfaceProperties;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_GfxContext.vkPhysicalDevice, m_GfxContext.surface, &surfaceProperties));

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_GfxContext.vkPhysicalDevice, m_GfxContext.surface, &formatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> formats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_GfxContext.vkPhysicalDevice, m_GfxContext.surface, &formatCount, formats.data());

	VkSurfaceFormatKHR format;
	if (formatCount == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		// There is no preferred format, so pick a default one
		format = formats[0];
		format.format = VK_FORMAT_B8G8R8A8_UNORM;
	}
	else
	{
		if (formatCount == 0)
		{
			throw std::runtime_error("Surface has no formats.");
		}

		format.format = VK_FORMAT_UNDEFINED;
		for (auto& candidate : formats)
		{
			switch (candidate.format)
			{
			case VK_FORMAT_R8G8B8A8_UNORM:
			case VK_FORMAT_B8G8R8A8_UNORM:
			case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
				format = candidate;
				break;

			default:
				break;
			}

			if (format.format != VK_FORMAT_UNDEFINED)
			{
				break;
			}
		}

		if (format.format == VK_FORMAT_UNDEFINED)
		{
			format = formats[0];
		}
	}

	VkExtent2D swapchainSize;
	if (surfaceProperties.currentExtent.width == 0xFFFFFFFF)
	{
		swapchainSize.width = m_GfxContext.swapchainDimensions.width;
		swapchainSize.height = m_GfxContext.swapchainDimensions.height;
	}
	else
	{
		swapchainSize = surfaceProperties.currentExtent;
	}

	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	uint32_t desiredSwapchainImages = surfaceProperties.minImageCount + 1;
	if ((surfaceProperties.maxImageCount > 0) && (desiredSwapchainImages > surfaceProperties.maxImageCount))
	{
		desiredSwapchainImages = surfaceProperties.maxImageCount;
	}

	VkSurfaceTransformFlagBitsKHR preTransform;
	if (surfaceProperties.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		preTransform = surfaceProperties.currentTransform;
	}

	VkSwapchainKHR oldSwapchain = m_GfxContext.swapchain;
	VkCompositeAlphaFlagBitsKHR composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	if (surfaceProperties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	}
	else if (surfaceProperties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	}
	else if (surfaceProperties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
	}
	else if (surfaceProperties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
	}

	VkSwapchainCreateInfoKHR info{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	info.surface = m_GfxContext.surface;
	info.minImageCount = desiredSwapchainImages;
	info.imageFormat = format.format;
	info.imageColorSpace = format.colorSpace;
	info.imageExtent.width = swapchainSize.width;
	info.imageExtent.height = swapchainSize.height;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.preTransform = preTransform;
	info.compositeAlpha = composite;
	info.presentMode = swapchainPresentMode;
	info.clipped = true;
	info.oldSwapchain = oldSwapchain;

	VK_CHECK(vkCreateSwapchainKHR(m_GfxContext.device, &info, nullptr, &m_GfxContext.swapchain));
	if (oldSwapchain != VK_NULL_HANDLE)
	{
		for (VkImageView imageView: m_GfxContext.swapchainImageViews)
		{
			vkDestroyImageView(m_GfxContext.device, imageView, nullptr);
		}

		uint32_t imageCount;
		VK_CHECK(vkGetSwapchainImagesKHR(m_GfxContext.device, oldSwapchain, &imageCount, nullptr));

		for (size_t i = 0; i < imageCount; i++)
		{
			//teardown_per_frame
		}

		m_GfxContext.swapchainImageViews.clear();

		vkDestroySwapchainKHR(m_GfxContext.device, oldSwapchain, nullptr);
	}

	m_GfxContext.swapchainDimensions = { swapchainSize.width, swapchainSize.height, format.format };
	uint32_t imageCount;
	VK_CHECK(vkGetSwapchainImagesKHR(m_GfxContext.device, m_GfxContext.swapchain, &imageCount, nullptr));

	std::vector<VkImage> swapChainImages(imageCount);
	VK_CHECK(vkGetSwapchainImagesKHR(m_GfxContext.device, m_GfxContext.swapchain, &imageCount, swapChainImages.data()));

	m_GfxContext.perFrame.clear();
	m_GfxContext.perFrame.resize(imageCount);

	for (size_t i = 0; i < imageCount; i++)
	{
		InitPerFrame(m_GfxContext.perFrame[i]);
	}

	for (size_t i = 0; i < imageCount; i++)
	{
		// Create an image view which we can render into.
		VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = m_GfxContext.swapchainDimensions.format;
		viewInfo.image = swapChainImages[i];
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;

		VkImageView imageView;
		VK_CHECK(vkCreateImageView(m_GfxContext.device, &viewInfo, nullptr, &imageView));

		m_GfxContext.swapchainImageViews.push_back(imageView);
	}
}

void GfxDeviceVulkan::InitRenderPass()
{
	VkAttachmentDescription attachment = { 0 };
	attachment.format = m_GfxContext.swapchainDimensions.format;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpass = { 0 };
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;

	VkSubpassDependency dependency = { 0 };
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo rpInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	rpInfo.attachmentCount = 1;
	rpInfo.pAttachments = &attachment;
	rpInfo.subpassCount = 1;
	rpInfo.pSubpasses = &subpass;
	rpInfo.dependencyCount = 1;
	rpInfo.pDependencies = &dependency;

	VK_CHECK(vkCreateRenderPass(m_GfxContext.device, &rpInfo, nullptr, &m_GfxContext.renderPass));
}

void GfxDeviceVulkan::InitPipeline()
{
	VkPipelineLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	VK_CHECK(vkCreatePipelineLayout(m_GfxContext.device, &layoutInfo, nullptr, &m_GfxContext.pipelineLayout));

	VkPipelineVertexInputStateCreateInfo vertexInput{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Specify rasterization state.
	VkPipelineRasterizationStateCreateInfo raster{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	raster.cullMode = VK_CULL_MODE_BACK_BIT;
	raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
	raster.lineWidth = 1.0f;

	// Our attachment will write to all color channels, but no blending is enabled.
	VkPipelineColorBlendAttachmentState blendAttachment{};
	blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo blend{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	blend.attachmentCount = 1;
	blend.pAttachments = &blendAttachment;

	// We will have one viewport and scissor box.
	VkPipelineViewportStateCreateInfo viewport{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewport.viewportCount = 1;
	viewport.scissorCount = 1;

	// Disable all depth testing.
	VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

	// No multisampling.
	VkPipelineMultisampleStateCreateInfo multisample{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Specify that these states will be dynamic, i.e. not part of pipeline state object.
	std::array<VkDynamicState, 2> dynamics{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamic{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamic.pDynamicStates = dynamics.data();
	dynamic.dynamicStateCount = dynamics.size();

	// Load our SPIR-V shaders.
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

	// Vertex stage of the pipeline
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = LoadShaderModule("C:/Wlon/WlonEngine/Code/Resources/triangle.vert");
	shaderStages[0].pName = "main";

	// Fragment stage of the pipeline
	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = LoadShaderModule("C:/Wlon/WlonEngine/Code/Resources/triangle.frag");
	shaderStages[1].pName = "main";

	VkGraphicsPipelineCreateInfo pipe{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipe.stageCount = shaderStages.size();
	pipe.pStages = shaderStages.data();
	pipe.pVertexInputState = &vertexInput;
	pipe.pInputAssemblyState = &inputAssembly;
	pipe.pRasterizationState = &raster;
	pipe.pColorBlendState = &blend;
	pipe.pMultisampleState = &multisample;
	pipe.pViewportState = &viewport;
	pipe.pDepthStencilState = &depthStencil;
	pipe.pDynamicState = &dynamic;

	// We need to specify the pipeline layout and the render pass description up front as well.
	pipe.renderPass = m_GfxContext.renderPass;
	pipe.layout = m_GfxContext.pipelineLayout;

	VK_CHECK(vkCreateGraphicsPipelines(m_GfxContext.device, VK_NULL_HANDLE, 1, &pipe, nullptr, &m_GfxContext.pipeline));

	vkDestroyShaderModule(m_GfxContext.device, shaderStages[0].module, nullptr);
	vkDestroyShaderModule(m_GfxContext.device, shaderStages[1].module, nullptr);
}

void GfxDeviceVulkan::InitFrameBuffers()
{
	VkDevice device = m_GfxContext.device;
	for (auto& imageView: m_GfxContext.swapchainImageViews)
	{
		VkFramebufferCreateInfo fbInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		fbInfo.renderPass = m_GfxContext.renderPass;
		fbInfo.attachmentCount = 1;
		fbInfo.pAttachments = &imageView;
		fbInfo.width = m_GfxContext.swapchainDimensions.width;
		fbInfo.height = m_GfxContext.swapchainDimensions.height;
		fbInfo.layers = 1;

		VkFramebuffer frameBuffer;
		VK_CHECK(vkCreateFramebuffer(device, &fbInfo, nullptr, &frameBuffer));
		m_GfxContext.swapchainFrameBuffers.push_back(frameBuffer);
	}
}

VkResult GfxDeviceVulkan::AcquireNextImage(uint32_t* image)
{
	VkSemaphore acquireSemaphore;
	if (m_GfxContext.recycledSemaphores.empty())
	{
		VkSemaphoreCreateInfo info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VK_CHECK(vkCreateSemaphore(m_GfxContext.device, &info, nullptr, &acquireSemaphore));
	}
	else
	{
		acquireSemaphore = m_GfxContext.recycledSemaphores.back();
		m_GfxContext.recycledSemaphores.pop_back();
	}
	// 等待acquireSemaphore完成后可取出image
	VkResult res = vkAcquireNextImageKHR(m_GfxContext.device, m_GfxContext.swapchain, UINT64_MAX, acquireSemaphore, VK_NULL_HANDLE, image);
	if (res != VK_SUCCESS)
	{
		m_GfxContext.recycledSemaphores.push_back(acquireSemaphore);
		return res;
	}

	if (m_GfxContext.perFrame[*image].queueSubmitFence != VK_NULL_HANDLE)
	{
		// 阻塞CPU，等待Fence完成
		vkWaitForFences(m_GfxContext.device, 1, &m_GfxContext.perFrame[*image].queueSubmitFence, true, UINT64_MAX);
		// 重置Fence为Unsignaled状态
		vkResetFences(m_GfxContext.device, 1, &m_GfxContext.perFrame[*image].queueSubmitFence);
	}

	if (m_GfxContext.perFrame[*image].primaryCommandPool != VK_NULL_HANDLE)
	{
		vkResetCommandPool(m_GfxContext.device, m_GfxContext.perFrame[*image].primaryCommandPool, 0);
	}

	VkSemaphore oldSemaphore = m_GfxContext.perFrame[*image].swapchainAcuireSemaphore;
	if (oldSemaphore != VK_NULL_HANDLE)
	{
		m_GfxContext.recycledSemaphores.push_back(oldSemaphore);
	}

	m_GfxContext.perFrame[*image].swapchainAcuireSemaphore = acquireSemaphore;
	return VK_SUCCESS;
}

void GfxDeviceVulkan::Render(uint32_t index)
{
	VkFramebuffer frameBuffer = m_GfxContext.swapchainFrameBuffers[index];
	VkCommandBuffer cmd = m_GfxContext.perFrame[index].primaryCommandBuffer;

	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &beginInfo);

	VkClearValue clearValue;
	clearValue.color = { {0.1f, 0.1f, 0.2f, 1.0f} };
	
	VkRenderPassBeginInfo rpBegin{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	rpBegin.renderPass = m_GfxContext.renderPass;
	rpBegin.framebuffer = frameBuffer;
	rpBegin.renderArea.extent.width = m_GfxContext.swapchainDimensions.width;
	rpBegin.renderArea.extent.height = m_GfxContext.swapchainDimensions.height;
	rpBegin.clearValueCount = 1;
	rpBegin.pClearValues = &clearValue;

	vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GfxContext.pipeline);

	VkViewport vp{};
	vp.width = static_cast<float>(m_GfxContext.swapchainDimensions.width);
	vp.height = static_cast<float>(m_GfxContext.swapchainDimensions.height);
	vp.minDepth = 0.0f;
	vp.maxDepth = 1.0f;
	vkCmdSetViewport(cmd, 0, 1, &vp);

	VkRect2D scissor{};
	scissor.extent.width = m_GfxContext.swapchainDimensions.width;
	scissor.extent.height = m_GfxContext.swapchainDimensions.height;
	vkCmdSetScissor(cmd, 0, 1, &scissor);
	
	vkCmdDraw(cmd, 3, 1, 0, 0);

	vkCmdEndRenderPass(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	if (m_GfxContext.perFrame[index].swapchainReleaseSemaphore == VK_NULL_HANDLE)
	{
		VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		VK_CHECK(vkCreateSemaphore(m_GfxContext.device, &semaphoreInfo, nullptr, &m_GfxContext.perFrame[index].swapchainReleaseSemaphore));
	}

	VkPipelineStageFlags waitStage{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	
	VkSubmitInfo info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	// 调用vkQueueSubmit时，表示此次提交的所有命令执行到pWaitDstStageMask时要停下，要等到所有pWaitSemaphores中的Semaphore状态变为signaled才可以继续执行
	// 命令执行结束后，pSignalSemaphores所有的Semaphore状态设置为Signaled
	info.commandBufferCount = 1;
	info.pCommandBuffers = &cmd;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &m_GfxContext.perFrame[index].swapchainAcuireSemaphore;
	info.pWaitDstStageMask = &waitStage;
	info.signalSemaphoreCount = 1;
	info.pSignalSemaphores = &m_GfxContext.perFrame[index].swapchainReleaseSemaphore;
	// vkQueueSubmit执行完queue的命令后，queueSubmitFence这个Fence会被设置为Signaled状态
	VK_CHECK(vkQueueSubmit(m_GfxContext.queue, 1, &info, m_GfxContext.perFrame[index].queueSubmitFence));

	// vkQueueSubmit中同时用到了Fence和Semaphore的同步，Fence用于阻塞CPU直到Queue命令执行结束，Semaphore用于不同命令提交的同步（GPU与GPU）
	// 这里Semaphore所呈现的意思是要等到把颜色写入到颜色缓冲区后，queueSubmitFence为Signaled后才可以Present Image
}

VkResult GfxDeviceVulkan::PresentImage(uint32_t index)
{
	VkPresentInfoKHR present{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	present.swapchainCount = 1;
	present.pSwapchains = &m_GfxContext.swapchain;
	present.pImageIndices = &index;
	present.waitSemaphoreCount = 1;
	present.pWaitSemaphores = &m_GfxContext.perFrame[index].swapchainReleaseSemaphore;

	return vkQueuePresentKHR(m_GfxContext.queue, &present);
}

VkShaderModule GfxDeviceVulkan::LoadShaderModule(const char* path)
{
	auto& buffer = FileSystem::LoadFile(path);

	std::string fileExt = path;
	// Extract extension name from the glsl shader file
	fileExt = fileExt.substr(fileExt.find_last_of(".") + 1);

	VkShaderStageFlagBits shaderStageBits;
	if (fileExt == "vert")
	{
		shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT;
	}
	else if (fileExt == "frag")
	{
		shaderStageBits = VK_SHADER_STAGE_FRAGMENT_BIT;
	}
	else if (fileExt == "comp")
	{
		shaderStageBits = VK_SHADER_STAGE_COMPUTE_BIT;
	}
	else if (fileExt == "geom")
	{
		shaderStageBits = VK_SHADER_STAGE_GEOMETRY_BIT;
	}
	else if (fileExt == "tesc")
	{
		shaderStageBits = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	}
	else if (fileExt == "tese")
	{
		shaderStageBits = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	}

	std::vector<uint32_t> spriv;
	std::string           infoLog;

	if (!GlslCompiler::CompilerToSpriv(shaderStageBits, buffer, "main", {}, spriv, infoLog))
	{
		return VK_NULL_HANDLE;
	}

	VkShaderModuleCreateInfo moduleInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	moduleInfo.codeSize = spriv.size() * sizeof(uint32_t);
	moduleInfo.pCode = spriv.data();

	VkShaderModule shaderModule;
	VK_CHECK(vkCreateShaderModule(m_GfxContext.device, &moduleInfo, nullptr, &shaderModule));
	
	return shaderModule;
}

void GfxDeviceVulkan::InitPerFrame(PerFrame& perframe)
{
	VkFenceCreateInfo info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	VK_CHECK(vkCreateFence(m_GfxContext.device, &info, nullptr, &perframe.queueSubmitFence));

	VkCommandPoolCreateInfo cmdPoolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	cmdPoolInfo.queueFamilyIndex = m_GfxContext.graphicsQueueIndex;
	VK_CHECK(vkCreateCommandPool(m_GfxContext.device, &cmdPoolInfo, nullptr, &perframe.primaryCommandPool));

	VkCommandBufferAllocateInfo cmdBufferInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	cmdBufferInfo.commandPool = perframe.primaryCommandPool;
	cmdBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferInfo.commandBufferCount = 1;
	VK_CHECK(vkAllocateCommandBuffers(m_GfxContext.device, &cmdBufferInfo, &perframe.primaryCommandBuffer));
	perframe.device = m_GfxContext.device;
	perframe.queueIndex = m_GfxContext.graphicsQueueIndex;
}
