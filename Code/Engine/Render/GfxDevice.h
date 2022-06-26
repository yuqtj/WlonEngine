#pragma once

#include <unordered_map>

#include "Apps/BaseInclude.h"

class GfxDevice
{
public:
	GfxDevice();
	virtual void Update() = 0;

	void Begin();
	virtual void BeginFrame() {};

	void AddDeviceExtension(const char* ext, bool enabled) { m_DeviceExtensions[ext] = enabled; }
	void AddInstanceExtension(const char* ext, bool enabled) { m_InstanceExtenstions[ext] = enabled; }
protected:
	std::unordered_map<const char*, bool> m_DeviceExtensions;
	std::unordered_map<const char*, bool> m_InstanceExtenstions;
private:
	bool m_FrameBegin;
};
