#pragma once

/// @brief Helper macro to test the result of Vulkan calls which can return an error.
#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			/*LOGE("Detected Vulkan error: {}", vkb::to_string(err));*/ \
			abort();                                                \
		}                                                           \
	} while (0)