
#include "WindowsApp.h"

#include "Apps/window/GlfwWindow.h"


WindowsApp::WindowsApp(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{

}

BasicWindow* WindowsApp::CreateWlWindow()
{
	return WL_NEW(GlfwWindow)(WindowsProperty());
}
