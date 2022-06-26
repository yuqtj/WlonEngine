#pragma once

#include "Render/RenderContext.h"
#include "Render/Vulkan/VulkanInclude.h"
#include <set>
#include <string>
#include <vector>

typedef std::set<std::string> ExtensionList;

struct VkRenderInstance
{
public:


	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue renderQueue;
	VkQueue presentQueue;
	uint32_t queueFamilyIndex;

	std::vector<const char*> EnabledLayerNames;
	ExtensionList EnabledInstanceExtensions;
	ExtensionList EnabledDeviceExtension;
	//VkQueue

	VkPhysicalDeviceFeatures EnabledFeatures;
	bool EnabledFeatureDecodeModeSharedExponent;

	bool HasDeviceExtension(const char* extName) const;
	bool HasInstanceExtension(const char* extName) const;
};

class VkContext : public RenderContext
{
public:
	virtual void Init() override;

	VkInstance CreateInstance(const ExtensionList* extraExtensions = nullptr);
	void CreateDevice(VkInstance instance, VkDevice* device, VkPhysicalDevice* physicalDevice, uint32_t* queueFamilyIndex, const ExtensionList& extraExtensions, uint32_t graphicsQueueCount);

protected:
private:
};