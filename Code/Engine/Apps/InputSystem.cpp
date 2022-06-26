
#include "InputSystem.h"

InputSystem InputSystem::s_Instance;

InputSystem& InputSystem::GetInstance()
{
	return s_Instance;
}

void InputSystem::Initialized()
{
	GetInstance();
}
