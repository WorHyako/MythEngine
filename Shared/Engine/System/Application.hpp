#pragma once

#include <System/ApplicationInterface.hpp>
#include <RHI/RHICommon/RHIModuleWrapper.hpp>
#include <Camera/TestCamera.hpp>
#include "Utils/UtilsFPS.hpp"

class IRHIModule;

class Application : public mythSystem::ApplicationInterface
{
public:
	Application();
	virtual ~Application();

	void CreateWindowGLFW();

private:
	RHI::IWindow* window_;
	RHI::IRHIModule* rhiModule_;
	CameraPositioner_FirstPerson positioner_;
	TestCamera camera_;
	FramesPerSecondCounter fpsCounter_;
};
