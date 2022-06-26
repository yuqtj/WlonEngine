
#include "VkContext.h"
#include "Apps/BaseInclude.h"

#include <vector>

VkRenderInstance s_Instance;
VkDebugReportCallbackEXT s_Debug = VK_NULL_HANDLE;
ExtensionList s_TotalRequestedDeviceExtensions;

void BuildEnabledLayerNames(const std::vector<VkLayerProperties>& layerProperties, std::vector<const char*>& enabledLayerNames)
{
	enabledLayerNames.clear();

	std::set<std::string> installedLayerNames;
	for (const auto& prop: layerProperties)
	{
		installedLayerNames.insert(prop.layerName);
	}

	auto IsLayerInstalled = [&](const char* layerName) { return installedLayerNames.find(layerName) != installedLayerNames.end(); };

	// Support Renderdoc
	const char* renderDocLayerName = "VK_LAYER_RENDERDOC_Capture";
	if (IsLayerInstalled(renderDocLayerName))
	{
		// ...
	}

	// If standard validation is available, just use that, otherwise explicitly set layernames
	const char* khronosValidationLayerName = "VK_LAYER_KHRONOS_validation";
	if (IsLayerInstalled(khronosValidationLayerName))
	{
		enabledLayerNames.push_back(khronosValidationLayerName);
	}
	else
	{
		const char* standardValidationLayerName = "VK_LAYER_LUNARG_standard_validation";
		if (IsLayerInstalled(standardValidationLayerName))
		{
			enabledLayerNames.push_back(standardValidationLayerName);
		}
		else
		{
			// Parse individual layers in specific order and as they're available, include them
			const char* validationLayerNames[] =
			{
				"VK_LAYER_GOOGLE_threading",
				"VK_LAYER_LUNARG_parameter_validation", // should be early in the list
				"VK_LAYER_LUNARG_object_tracker",
				"VK_LAYER_LUNARG_core_validation",
				"VK_LAYER_LUNARG_device_limits",
				"VK_LAYER_LUNARG_image",
				"VK_LAYER_LUNARG_swapchain",
				"VK_LAYER_GOOGLE_unique_objects", // should be late in the list, gets data early from the driver
			};

			for (const char* layerName: validationLayerNames)
			{
				if (IsLayerInstalled(layerName))
				{
					enabledLayerNames.push_back(layerName);
				}
			}
		}
	}

	for (const char* layerName: enabledLayerNames)
	{
		std::cout << "Layer enabled: " << layerName << std::endl;
	}
}

void AppendExtensionOfLayer(const char* layerName, std::vector<VkExtensionProperties>& extensions)
{
	uint32_t instanceExtensionPropertiesCount = 0;
	vkEnumerateInstanceExtensionProperties(layerName, &instanceExtensionPropertiesCount, nullptr);

	if (instanceExtensionPropertiesCount == 0)
	{
		return;
	}

	const size_t oldSize = extensions.size();
	extensions.resize(oldSize + instanceExtensionPropertiesCount);
	vkEnumerateInstanceExtensionProperties(layerName, &instanceExtensionPropertiesCount, &extensions[oldSize]);
}

void BuildEnabledExtensions(const std::vector<VkExtensionProperties>& extensionProperties, const ExtensionList& requestedExtensions, ExtensionList& enabledExtensions)
{
	enabledExtensions.clear();

	std::cout << "Extensions: count = " << extensionProperties.size() << std::endl;
	for (const auto& prop: extensionProperties)
	{
		std::string name(prop.extensionName);
		bool enabled = false;
		if (requestedExtensions.find(name) != requestedExtensions.end())
		{
			enabledExtensions.insert(name);
			enabled = true;
		}

		std::cout << "Extensions: name = " << name.c_str() << ", enabled = %d" << enabled << std::endl;
	}
}

VkPhysicalDevice SelectPhysicalDevice(VkInstance instance, int requestedDeviceIndex)
{
	uint32_t physicalDeviceCount = 0;
	VkResult res = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL);
	if (physicalDeviceCount < 1)
	{
		return VK_NULL_HANDLE;
	}

	std::vector<VkPhysicalDevice> physicalDevices;
	physicalDevices.resize(physicalDeviceCount);
	res = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

	std::vector<VkPhysicalDeviceProperties> physicalDeviceProperties;
	physicalDeviceProperties.resize(physicalDeviceCount);

	for (uint32_t i = 0; i < physicalDeviceCount; ++i)
	{
		const VkPhysicalDevice device = physicalDevices[i];
		VkPhysicalDeviceProperties& prop = physicalDeviceProperties[i];
		vkGetPhysicalDeviceProperties(device, &prop);
	}

	if (requestedDeviceIndex >= 0 && requestedDeviceIndex < physicalDevices.size())
	{
		return physicalDevices[requestedDeviceIndex];
	}

	const VkPhysicalDeviceType deviceTypePriority[] =
	{
		VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
		VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
		VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
		VK_PHYSICAL_DEVICE_TYPE_OTHER
	};

	for (const VkPhysicalDeviceType deviceType: deviceTypePriority)
	{
		auto it = std::find_if(physicalDeviceProperties.begin(), physicalDeviceProperties.end(), [deviceType](const VkPhysicalDeviceProperties& prop)
		{
			return prop.deviceType == deviceType;
		});
		if (it != physicalDeviceProperties.end())
		{
			return physicalDevices[std::distance(physicalDeviceProperties.begin(), it)];
		}
	}

	return VK_NULL_HANDLE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugKHRDefaultCallback(
	VkDebugReportFlagsEXT msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location, int32_t msgCode,
	const char* pLayerPrefix, const char* pMsg, void* pUserData)
{
	std::string message;

	if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		message += "ERROR: ";
	else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		message += "WARNING: ";
	else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		message += "PERFORMANCE WARNING: ";
	else if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
		message += "INFO: ";
	else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
		message += "DEBUG: ";

	if (msgCode == 2) // ignore FS writes to output location 0 with no matching attachment, we do this when rendering shadow maps when the fragment shader uses discard
		return false;

	if (msgCode == 15) // Ignore the warning, we can't rely correct load ops (vkCmdBeginRenderPass(): Cannot read invalid region of memory ... please fill the memory before using.)
		return false;

	printf("VULKAN DEBUG: %s [%s, %d]: %s\n", message.c_str(), pLayerPrefix, msgCode, pMsg);

	return false;
}

VkDebugReportCallbackEXT CreateDebug(VkInstance instance)
{
	VkDebugReportCallbackEXT debug;

	PFN_vkCreateDebugReportCallbackEXT dbgCreateMsgCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (!dbgCreateMsgCallback)
	{
		return VK_NULL_HANDLE;
	}

	VkDebugReportCallbackCreateInfoEXT createInfo =
	{
		VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
		NULL,
		VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,
		&DebugKHRDefaultCallback,
		NULL
	};

	VkResult res = dbgCreateMsgCallback(instance, &createInfo, NULL, &debug);

	return debug;
}

void VkContext::Init()
{
	uint32_t graphicsQueueCount = 1;
	s_Instance.instance = CreateInstance();
	ExtensionList extraExtensions;

	if (s_Instance.HasInstanceExtension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
	{
		s_Debug = CreateDebug(s_Instance.instance);
	}

	CreateDevice(s_Instance.instance, &s_Instance.device, &s_Instance.physicalDevice, &s_Instance.queueFamilyIndex, extraExtensions, graphicsQueueCount);

	vkGetDeviceQueue(s_Instance.device, s_Instance.queueFamilyIndex, 0, &s_Instance.renderQueue);
	s_Instance.presentQueue = s_Instance.renderQueue;
}

VkInstance VkContext::CreateInstance(const ExtensionList* extraExtensions /*= nullptr*/)
{
	VkApplicationInfo applicationInfo = {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pNext = nullptr;
	applicationInfo.pApplicationName = "WEngine";
	applicationInfo.applicationVersion = 1;
	applicationInfo.pEngineName = "WEngine";
	applicationInfo.engineVersion = 1;
	applicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

	uint32_t instanceLayerPropertyCount = 0;
	VkResult res = vkEnumerateInstanceLayerProperties(&instanceLayerPropertyCount, nullptr);
	std::vector<VkLayerProperties> instanceLayerProperties{};
	if (instanceLayerPropertyCount > 0)
	{
		instanceLayerProperties.resize(instanceLayerPropertyCount);
		res = vkEnumerateInstanceLayerProperties(&instanceLayerPropertyCount, &instanceLayerProperties[0]);
		VK_CHECK(res);
	}

	BuildEnabledLayerNames(instanceLayerProperties, s_Instance.EnabledLayerNames);

	// Extensions
	std::vector<VkExtensionProperties> instanceExtensionProperties;
	AppendExtensionOfLayer(nullptr, instanceExtensionProperties);
	for (const auto& layerName: s_Instance.EnabledLayerNames)
	{
		AppendExtensionOfLayer(layerName, instanceExtensionProperties);
	}

	std::string kInstanceExtensionsWeWant[] =
	{
		// debug
		//VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_DISPLAY_EXTENSION_NAME,
		// win32
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		// android
		//VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
		
		VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME,
		// ?
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
	};

	ExtensionList totalRequestedInstanceExtensions;
	totalRequestedInstanceExtensions.insert(std::begin(kInstanceExtensionsWeWant), std::end(kInstanceExtensionsWeWant));
	if (extraExtensions)
	{
		totalRequestedInstanceExtensions.insert(std::begin(*extraExtensions), std::end(*extraExtensions));
	}

	BuildEnabledExtensions(instanceExtensionProperties, totalRequestedInstanceExtensions, s_Instance.EnabledInstanceExtensions);

	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.flags = 0;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;
	instanceCreateInfo.enabledLayerCount = s_Instance.EnabledLayerNames.size();
	instanceCreateInfo.ppEnabledLayerNames = s_Instance.EnabledLayerNames.size() ? s_Instance.EnabledLayerNames.data() : nullptr;
	
	std::vector<const char*> extensionNames;
	for (const auto& extension: s_Instance.EnabledInstanceExtensions)
	{
		extensionNames.push_back(extension.c_str());
	}

	instanceCreateInfo.enabledExtensionCount = extensionNames.size();
	instanceCreateInfo.ppEnabledExtensionNames = extensionNames.size() ? extensionNames.data() : nullptr;

	VkInstance instance;
	res = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
	VK_CHECK(res);

	return res == VK_SUCCESS ? instance : VK_NULL_HANDLE;
}

uint32_t FindQueueFamilyIndex(std::vector<VkQueueFamilyProperties> const & queueFamilyProperties, VkQueueFlagBits queueFlagBits)
{
	uint32_t index = 0;
	for (const auto& property : queueFamilyProperties)
	{
		if (property.queueFlags & queueFlagBits)
		{
			return index;
		}

		++index;
	}

	return -1;
}

void EnablePhysicalDeviceFeatures(const VkPhysicalDeviceFeatures& available, VkPhysicalDeviceFeatures& enabled)
{
	enabled.imageCubeArray = available.imageCubeArray;
	enabled.geometryShader = available.geometryShader;
	enabled.tessellationShader = available.tessellationShader;
	enabled.logicOp = available.logicOp;
	enabled.depthClamp = available.depthClamp;
	enabled.fillModeNonSolid = available.fillModeNonSolid;
	enabled.samplerAnisotropy = available.samplerAnisotropy;
	enabled.fragmentStoresAndAtomics = available.fragmentStoresAndAtomics;
	enabled.sampleRateShading = available.sampleRateShading;

	enabled.independentBlend = available.independentBlend;

	enabled.textureCompressionETC2 = available.textureCompressionETC2;
	enabled.textureCompressionASTC_LDR = available.textureCompressionASTC_LDR;
	enabled.textureCompressionBC = available.textureCompressionBC;

	enabled.shaderImageGatherExtended = available.shaderImageGatherExtended;
	enabled.shaderStorageImageExtendedFormats = available.shaderStorageImageExtendedFormats;
	enabled.shaderStorageImageMultisample = available.shaderStorageImageMultisample;
	enabled.shaderStorageImageReadWithoutFormat = available.shaderStorageImageReadWithoutFormat;
	enabled.shaderStorageImageWriteWithoutFormat = available.shaderStorageImageWriteWithoutFormat;
	enabled.shaderUniformBufferArrayDynamicIndexing = available.shaderUniformBufferArrayDynamicIndexing;
	enabled.shaderSampledImageArrayDynamicIndexing = available.shaderSampledImageArrayDynamicIndexing;
	enabled.shaderStorageBufferArrayDynamicIndexing = available.shaderStorageBufferArrayDynamicIndexing;
	enabled.shaderStorageImageArrayDynamicIndexing = available.shaderStorageImageArrayDynamicIndexing;
	enabled.shaderClipDistance = available.shaderClipDistance;
	enabled.shaderCullDistance = available.shaderCullDistance;
	enabled.shaderFloat64 = available.shaderFloat64;
	enabled.shaderInt64 = available.shaderInt64;
	enabled.shaderResourceResidency = available.shaderResourceResidency;
	enabled.shaderResourceMinLod = available.shaderResourceMinLod;

	enabled.shaderTessellationAndGeometryPointSize = available.shaderTessellationAndGeometryPointSize;

	enabled.sparseBinding = available.sparseBinding;
	enabled.sparseResidencyBuffer = available.sparseResidencyBuffer;
	enabled.sparseResidencyImage2D = available.sparseResidencyImage2D;
	enabled.sparseResidencyImage3D = available.sparseResidencyImage3D;
	enabled.sparseResidency2Samples = available.sparseResidency2Samples;
	enabled.sparseResidency4Samples = available.sparseResidency4Samples;
	enabled.sparseResidency8Samples = available.sparseResidency8Samples;
	enabled.sparseResidency16Samples = available.sparseResidency16Samples;
	enabled.sparseResidencyAliased = available.sparseResidencyAliased;
}

void VkContext::CreateDevice(VkInstance instance, VkDevice* device, VkPhysicalDevice* physicalDevice, uint32_t* queueFamilyIndex, const ExtensionList& extraExtensions, uint32_t graphicsQueueCount)
{
	*physicalDevice = SelectPhysicalDevice(instance, -1);
	
	// List queue families
	uint32_t queueFamilyPropertyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(*physicalDevice, &queueFamilyPropertyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(*physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

	*queueFamilyIndex = FindQueueFamilyIndex(queueFamilyProperties, VK_QUEUE_GRAPHICS_BIT);

	if (graphicsQueueCount > 1 && *queueFamilyIndex != -1)
	{
		graphicsQueueCount = (std::min)(graphicsQueueCount, queueFamilyProperties[*queueFamilyIndex].queueCount);
	}

	VkDeviceQueueCreateInfo deviceQueueCreateInfo = {};
	deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfo.pNext = nullptr;
	deviceQueueCreateInfo.queueCount = graphicsQueueCount;
	deviceQueueCreateInfo.queueFamilyIndex = *queueFamilyIndex;
	std::vector<float> QueuePriorities(graphicsQueueCount, 0.0f);
	deviceQueueCreateInfo.pQueuePriorities = QueuePriorities.data();

	VkPhysicalDeviceFeatures2 availableFeatures{};
	availableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	availableFeatures.pNext = nullptr;

#ifdef VK_EXT_astc_decode_mode
	VkPhysicalDeviceASTCDecodeFeaturesEXT featureAstcDecodeMode{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ASTC_DECODE_FEATURES_EXT, nullptr, VK_FALSE };
	featureAstcDecodeMode.pNext = availableFeatures.pNext;
	availableFeatures.pNext = &featureAstcDecodeMode;
#endif

	if (s_Instance.HasInstanceExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
	{
		vkGetPhysicalDeviceFeatures2KHR(*physicalDevice, &availableFeatures);
	}
	else
	{
		vkGetPhysicalDeviceFeatures(s_Instance.physicalDevice, &availableFeatures.features);
	}

	memset(&s_Instance.EnabledFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
	EnablePhysicalDeviceFeatures(availableFeatures.features, s_Instance.EnabledFeatures);

	// Device Layers initialization
	uint32_t deviceLayerPropertyCount = 0;
	VkResult res = vkEnumerateDeviceLayerProperties(*physicalDevice, &deviceLayerPropertyCount, NULL);

	std::vector<VkLayerProperties> deviceLayerProperties(deviceLayerPropertyCount);
	res = vkEnumerateDeviceLayerProperties(*physicalDevice, &deviceLayerPropertyCount, &deviceLayerProperties[0]);

	// Device Extensions initialization
	uint32_t deviceExtensionPropertiesCount = 0;
	vkEnumerateDeviceExtensionProperties(*physicalDevice, NULL, &deviceExtensionPropertiesCount, NULL);

	std::vector<VkExtensionProperties> deviceExtensionProperties(deviceExtensionPropertiesCount);
	vkEnumerateDeviceExtensionProperties(*physicalDevice, NULL, &deviceExtensionPropertiesCount, &deviceExtensionProperties[0]);

	const char* kDeviceExtensionsWeWant[] =
	{
#if UNITY_DEVELOPER_BUILD
			VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
#endif
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME,
#ifdef VK_KHR_MAINTENANCE1_EXTENSION_NAME
			VK_KHR_MAINTENANCE1_EXTENSION_NAME,
#endif
#ifdef VK_KHR_MAINTENANCE2_EXTENSION_NAME
			VK_KHR_MAINTENANCE2_EXTENSION_NAME,
#endif
#ifdef VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
			VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
#endif
#ifdef VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME
			VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
#endif
#ifdef VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME
			VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
#endif
#ifdef VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME
			VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,
#endif
#ifdef VK_EXT_astc_decode_mode
			VK_EXT_ASTC_DECODE_MODE_EXTENSION_NAME,
#endif
#ifdef VK_KHR_multiview
			VK_KHR_MULTIVIEW_EXTENSION_NAME,
#endif
#ifdef VK_EXT_fragment_density_map
			VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME,
#endif
#ifdef VK_EXT_fragment_density_map2
			VK_EXT_FRAGMENT_DENSITY_MAP_2_EXTENSION_NAME,
#endif
#ifdef VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME
			VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME,
#endif
#ifdef VK_EXT_hdr_metadata
			VK_EXT_HDR_METADATA_EXTENSION_NAME,
#endif
#ifdef VK_EXT_conservative_rasterization
			VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME,
#endif
#ifdef VK_KHR_create_renderpass2
			VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
#endif
#ifdef VK_KHR_depth_stencil_resolve
			VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
#endif
#ifdef VK_EXT_texture_compression_astc_hdr
			VK_EXT_TEXTURE_COMPRESSION_ASTC_HDR_EXTENSION_NAME,
#endif
	};

	s_TotalRequestedDeviceExtensions.insert(std::begin(kDeviceExtensionsWeWant), std::end(kDeviceExtensionsWeWant));
	s_TotalRequestedDeviceExtensions.insert(std::begin(extraExtensions), std::end(extraExtensions));

#if PLATFORM_ANDROID
#endif

	BuildEnabledExtensions(deviceExtensionProperties, s_TotalRequestedDeviceExtensions, s_Instance.EnabledDeviceExtension);

	void* deviceCreateNextPtr = NULL;
#ifdef VK_KHR_get_physical_device_properties2
	void** nextPtrPtr = &deviceCreateNextPtr;

	#ifdef VK_EXT_fragment_density_map
	VkPhysicalDeviceFragmentDensityMapFeaturesEXT fragmentDensityMapFeature = {};
	if (s_Instance.HasDeviceExtension(VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME))
	{
		fragmentDensityMapFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT;

		*nextPtrPtr = &fragmentDensityMapFeature;
		nextPtrPtr = &fragmentDensityMapFeature.pNext;
	}
	#endif

	#ifdef VK_EXT_fragment_density_map2
	VkPhysicalDeviceFragmentDensityMap2FeaturesEXT fragmentDensityMap2Features = {};
	if (s_Instance.HasDeviceExtension(VK_EXT_FRAGMENT_DENSITY_MAP_2_EXTENSION_NAME))
	{
		fragmentDensityMap2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_FEATURES_EXT;

		*nextPtrPtr = &fragmentDensityMap2Features;
		nextPtrPtr = &fragmentDensityMap2Features.pNext;
	}
	#endif

	#ifdef VK_KHR_multiview
	VkPhysicalDeviceMultiviewFeatures multiviewFeature = {};
	if (s_Instance.HasDeviceExtension(VK_KHR_MULTIVIEW_EXTENSION_NAME))
	{
		multiviewFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
		*nextPtrPtr = &multiviewFeature;
		nextPtrPtr = &multiviewFeature.pNext;
	}
	#endif

	if (s_Instance.HasInstanceExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
	{
		VkPhysicalDeviceFeatures2KHR features2 = {};
		features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
		features2.pNext = deviceCreateNextPtr;

		//NOTE: passing deviceCreateNextPtr to deviceCreateInfo.pNext after this query will enable all supported features for extensions in this linked list
		vkGetPhysicalDeviceFeatures2KHR(*physicalDevice, &features2);
	}
	else
	{
		deviceCreateNextPtr = NULL;
	}
#endif

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = deviceCreateNextPtr;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
	deviceCreateInfo.enabledLayerCount = s_Instance.EnabledLayerNames.size();
	deviceCreateInfo.ppEnabledLayerNames = s_Instance.EnabledLayerNames.size() ? s_Instance.EnabledLayerNames.data() : NULL;

	auto count = s_Instance.EnabledDeviceExtension.size();
	std::vector<const char*> names(count);

	count = 0;
	for (const auto& extension: s_Instance.EnabledDeviceExtension)
	{
		names[count++] = extension.c_str();
	}

	deviceCreateInfo.enabledExtensionCount = count;
	deviceCreateInfo.ppEnabledExtensionNames = count ? names.data() : NULL;
	deviceCreateInfo.pEnabledFeatures = &s_Instance.EnabledFeatures;

#ifdef VK_EXT_astc_decode_mode
	if (featureAstcDecodeMode.decodeModeSharedExponent)
	{
		featureAstcDecodeMode.pNext = (void*)deviceCreateInfo.pNext;
		deviceCreateInfo.pNext = &featureAstcDecodeMode;
		s_Instance.EnabledFeatureDecodeModeSharedExponent = true;
	}
#endif

	res = vkCreateDevice(*physicalDevice, &deviceCreateInfo, NULL, device);
	VK_CHECK(res);
}

inline bool ContainsString(const ExtensionList& strings, const std::string& string)
{
	return strings.find(string) != std::end(strings);
}

bool VkRenderInstance::HasDeviceExtension(const char* extName) const
{
	return ContainsString(EnabledDeviceExtension, extName);
}

bool VkRenderInstance::HasInstanceExtension(const char* extName) const
{
	return ContainsString(EnabledInstanceExtensions, extName);
}
