
#include "WinUtils.h"

static HINSTANCE gInstanceHandle = nullptr;

void WinUtils::SetInstanceHandle(HINSTANCE instance)
{
	gInstanceHandle = instance;
}

HINSTANCE WinUtils::GetInstanceHandle()
{
	return gInstanceHandle;
}




