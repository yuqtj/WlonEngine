#pragma once

#include "Render/Vulkan/VulkanInclude.h"
#include <set>

class GfxDevice;

struct SwapChainConfiguration
{
	NativeWindowHandle window;
	VkExtent2D realExtent;
	VkExtent2D extent;
};

class VKSwapChain
{
public:
	VKSwapChain(GfxDevice* gfxDevice, const VkExtent2D extent = {}, const uint32_t imageCount = 3,
		const VkSurfaceTransformFlagBitsKHR transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		const VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR,
		const std::set<VkImageUsageFlagBits>& image_usage_flags = { VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT });
protected:
private:

	void Create();

	VkFormat m_ColorFormat;
	//VkFormat m_

	VkInstance m_Instance;
	VkDevice m_Device;
	VkSurfaceKHR m_MainSurface;
	VkPhysicalDevice m_PhysicalDevice;
	uint32_t m_QueueFamilyIndex;

	SwapChainConfiguration m_Configuration;
};