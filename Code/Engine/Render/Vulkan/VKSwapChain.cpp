#include "VKSwapChain.h"
#include "Apps/window/WinUtils.h"

#if USE_WINDOWS
VkSurfaceKHR PlatformCreateSurface(VkInstance instance, NativeWindowHandle window, const VkExtent2D& requestedExtent)
{
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hwnd = window;
	surfaceCreateInfo.hinstance = WinUtils::GetInstanceHandle();
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkResult result = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
	return result == VK_SUCCESS ? surface : VK_NULL_HANDLE;
}
//#elif Android
#endif


VKSwapChain::VKSwapChain(GfxDevice* gfxDevice, const VkExtent2D extent /*= {}*/, const uint32_t imageCount /*= 3*/, const VkSurfaceTransformFlagBitsKHR transform /*= VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR*/, const VkPresentModeKHR presentMode /*= VK_PRESENT_MODE_FIFO_KHR*/, const std::set<VkImageUsageFlagBits>& image_usage_flags /*= { VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT }*/)
{
	m_Device = gfxDevice;
	m_MainSurface = VK_NULL_HANDLE;

	VkSurfaceCapabilitiesKHR surfaceCapabilities{};
	//vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gfxDevice->)
}

void VKSwapChain::Create()
{
	VkResult result = VK_SUCCESS;
	if (m_MainSurface == VK_NULL_HANDLE)
	{
		m_MainSurface = PlatformCreateSurface(m_Instance, m_Configuration.window, m_Configuration.extent);
	}

	VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
	if (m_MainSurface != VK_NULL_HANDLE)
	{
		result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_MainSurface, &surfaceCapabilities);
		DebugAssert(result == VK_SUCCESS);

		VkBool32 supportsPresentation = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, m_QueueFamilyIndex, m_MainSurface, &supportsPresentation);
		DebugAssert(supportsPresentation);
	}

	if (surfaceCapabilities.currentExtent.height == 0 && surfaceCapabilities.currentExtent.width == 0)
	{
		if (m_MainSurface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(m_Instance, m_MainSurface, nullptr);
			m_MainSurface = VK_NULL_HANDLE;
		}

		if (m_Configuration.extent.width == 0 || m_Configuration.extent.height == 0)
		{
			//m_Configuration.extent = 
		}
				
	}
}
