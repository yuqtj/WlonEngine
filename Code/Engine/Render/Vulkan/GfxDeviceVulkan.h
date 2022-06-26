
#pragma once

#include "Render/GfxDevice.h"
#include "Render/Vulkan/VulkanInclude.h"
#include <vector>

class BasicWindow;
class VKSwapChain;

class GfxDeviceVulkan : public GfxDevice
{
public:
	/**
	 * @brief Swapchain state
	 */
	struct SwapchainDimensions
	{
		/// Width of the swapchain.
		uint32_t width = 0;

		/// Height of the swapchain.
		uint32_t height = 0;

		/// Pixel format of the swapchain.
		VkFormat format = VK_FORMAT_UNDEFINED;
	};

	struct PerFrame
	{
		VkDevice device = VK_NULL_HANDLE;
		// Fence在命令队列上的操作同步
		VkFence queueSubmitFence = VK_NULL_HANDLE;

		VkCommandPool primaryCommandPool = VK_NULL_HANDLE;

		VkCommandBuffer primaryCommandBuffer = VK_NULL_HANDLE;

		VkSemaphore swapchainAcuireSemaphore = VK_NULL_HANDLE;

		VkSemaphore swapchainReleaseSemaphore = VK_NULL_HANDLE;

		int32_t queueIndex;
	};

	struct GfxContext
	{
		VkInstance instance = VK_NULL_HANDLE;

		VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;

		VkDevice device = VK_NULL_HANDLE;

		VkQueue queue = VK_NULL_HANDLE;

		VkSwapchainKHR swapchain = VK_NULL_HANDLE;

		SwapchainDimensions swapchainDimensions;

		VkSurfaceKHR surface = VK_NULL_HANDLE;

		int32_t graphicsQueueIndex = -1;

		std::vector<VkImageView> swapchainImageViews;

		std::vector<VkFramebuffer> swapchainFrameBuffers;

		VkRenderPass renderPass = VK_NULL_HANDLE;

		VkPipeline pipeline = VK_NULL_HANDLE;

		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

		VkDebugReportCallbackEXT debugCallback = VK_NULL_HANDLE;

		std::vector<VkSemaphore> recycledSemaphores;

		std::vector<PerFrame> perFrame;
	};

	GfxDeviceVulkan(BasicWindow* window);
	~GfxDeviceVulkan();

	void Initialized();
	virtual void Update();


	virtual void BeginFrame() override;

private:
	void InitInstance();
	void InitDevice();
	void InitSwapchain(uint32_t width, uint32_t height);
	void InitRenderPass();
	void InitPipeline();
	void InitFrameBuffers();

	VkResult AcquireNextImage(uint32_t* image);
	void Render(uint32_t index);
	VkResult PresentImage(uint32_t index);

	VkShaderModule LoadShaderModule(const char* path);

	void InitPerFrame(PerFrame& perframe);

	GfxContext m_GfxContext;
	VKSwapChain* m_PrimarySwapChain;
	
};

