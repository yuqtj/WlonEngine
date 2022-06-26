#pragma once

#include "Apps/Error.h"
#include <iostream>

#if USE_WINDOWS

#include <Windows.h>

#endif

template<typename T>
inline void DeleteInternal(T* ptr)
{
	if (ptr)
	{
		delete ptr;
		ptr = nullptr;
	}
}

#define WL_NEW(type)    new type     //new (alignof(type), __FILE_STRIPPED__, __LINE__) type
#define WL_DELETE(ptr)  { DeleteInternal(ptr); }


#define DebugAssert(x) assert(x)

#include <stdexcept>



//#define GFX_DEBUG