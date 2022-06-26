#pragma once

#include <string>
#include "Apps/BaseInclude.h"

enum class Mode
{
	Headless,
	Fullscreen,
	FullscreenBorderless,
	Default
};

struct Extent
{
	uint32_t width;
	uint32_t height;
};

struct WindowsProperty
{
	Mode        mode = Mode::Default;
	std::string title = "";
	Extent      extent = { 1280, 720 };
};