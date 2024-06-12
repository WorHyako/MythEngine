#pragma once

#include <EngineTypes.hpp>

struct WindowParameters
{
	uint16_t Width = 1280;
	uint16_t Height = 720;
	std::string WindowTitle = "Default D3D12 Window";
    uint16_t GraphicsInterface = 0;
};
