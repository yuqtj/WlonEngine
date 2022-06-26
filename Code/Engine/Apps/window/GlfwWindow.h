
#include "Apps/window/BasicWindow.h"
#include "Apps/BaseInclude.h"
#include "volk.h"

struct GLFWwindow;

class GlfwWindow : public BasicWindow
{
public:
	GlfwWindow(WindowsProperty param);
	VkSurfaceKHR CreateSurface(VkInstance& instance);
	virtual bool ShouldClose();
	virtual void ProcessEvents();
protected:
private:
	GLFWwindow* m_HandleWindow;
};