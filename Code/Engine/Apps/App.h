#pragma once

#include "Apps/BaseInclude.h"

class BasicWindow;

enum class ExitCode
{
	Success = 0, /* App executed as expected */
	Help,        /* App should show help */
	Close,       /* App has been requested to close at initialization */
	FatalError   /* App encountered an unexpected error */
};

// Basic window

class App
{
public:
	virtual ExitCode Initialize();
	ExitCode PlayerLoop();
	void Terminate();

protected:
	virtual BasicWindow* CreateWlWindow() = 0;

	BasicWindow* m_AppWindow;
private:

};