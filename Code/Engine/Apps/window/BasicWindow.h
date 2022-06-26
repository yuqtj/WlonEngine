#pragma once

#include <string>
#include "Apps/window/WindowInclude.h"

class BasicWindow
{
public:
	BasicWindow(WindowsProperty property) { m_Property = property; };
	inline WindowsProperty& GetWindowsProperty() { return m_Property; };
	virtual bool ShouldClose() = 0;
	virtual void ProcessEvents() {};
protected:
	WindowsProperty m_Property;
private:
};