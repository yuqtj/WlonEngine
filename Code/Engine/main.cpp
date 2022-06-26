

#include "Apps/BaseInclude.h"


#if USE_WINDOWS


#include "Apps/platform/WindowsApp.h"

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	WindowsApp app(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
	auto code = app.Initialize();

	while (code == ExitCode::Success)
	{
		app.PlayerLoop();
	}

	app.Terminate();

	return EXIT_SUCCESS;
}


#else

#endif