
#include "GlfwWindow.h"


// glfw include		
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

void ErrorCallback(int error, const char* description)
{
	//LOGE("GLFW Error (code {}): {}", error, description);
}

void CloseCallback(GLFWwindow* window)
{
	glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void KeyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{

}

GlfwWindow::GlfwWindow(WindowsProperty param) : BasicWindow(param)
{
	if (!glfwInit())
	{
		throw std::runtime_error("GLFW couldn't be initialized.");
	}

	glfwSetErrorCallback(ErrorCallback);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	switch (param.mode)
	{
	case Mode::Fullscreen:
	{
		auto* monitor = glfwGetPrimaryMonitor();
		const auto* mode = glfwGetVideoMode(monitor);
		m_HandleWindow = glfwCreateWindow(mode->width, mode->height, param.title.c_str(), monitor, NULL);
		break;
	}

	case Mode::FullscreenBorderless: {
		auto* monitor = glfwGetPrimaryMonitor();
		const auto* mode = glfwGetVideoMode(monitor);
		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
		m_HandleWindow = glfwCreateWindow(mode->width, mode->height, param.title.c_str(), monitor, NULL);
		break;
	}

	default:
		m_HandleWindow = glfwCreateWindow(param.extent.width, param.extent.height, param.title.c_str(), NULL, NULL);
		break;

	}

	//resize(Extent{ param.extent.width, param.extent.height });

	if (!m_HandleWindow)
	{
		throw std::runtime_error("Couldn't create glfw window.");
	}

	//glfwSetWindowUserPointer(handle, platform);

	glfwSetWindowCloseCallback(m_HandleWindow, CloseCallback);
	//glfwSetWindowSizeCallback(handle, window_size_callback);
	/*glfwSetWindowFocusCallback(handle, window_focus_callback);
	glfwSetKeyCallback(handle, KeyCallback);
	glfwSetCursorPosCallback(handle, cursor_position_callback);
	glfwSetMouseButtonCallback(handle, mouse_button_callback);*/

	glfwSetInputMode(m_HandleWindow, GLFW_STICKY_KEYS, 1);
	glfwSetInputMode(m_HandleWindow, GLFW_STICKY_MOUSE_BUTTONS, 1);
}

VkSurfaceKHR GlfwWindow::CreateSurface(VkInstance& instance)
{
	if (instance == VK_NULL_HANDLE || !m_HandleWindow)
	{
		return VK_NULL_HANDLE;
	}

	VkSurfaceKHR surface;
	VkResult errCode = glfwCreateWindowSurface(instance, m_HandleWindow, nullptr, &surface);
	if (errCode != VK_SUCCESS)
	{
		return VK_NULL_HANDLE;
	}

	return surface;
}

bool GlfwWindow::ShouldClose()
{
	return glfwWindowShouldClose(m_HandleWindow);
}

void GlfwWindow::ProcessEvents()
{
	glfwPollEvents();
}

