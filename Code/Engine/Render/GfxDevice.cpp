
#include "GfxDevice.h"

GfxDevice::GfxDevice()
{
	m_FrameBegin = false;
}

void GfxDevice::Begin()
{
	if (!m_FrameBegin)
	{
		BeginFrame();
	}
}

