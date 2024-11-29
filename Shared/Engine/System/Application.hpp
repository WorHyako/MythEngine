#pragma once

#include <System/ApplicationInterface.hpp>
#include <RHI/RHICommon/RHIModuleWrapper.hpp>
#include <Camera/TestCamera.hpp>
#include "Utils/UtilsFPS.hpp"

class IRHIModule;

namespace mythSystem
{
	class Application : public ApplicationInterface
	{
	public:
		Application();
		virtual ~Application();

		UniquePtr<WindowInterface> CreateWindow() override;

		void CreateWindowGLFW();

		virtual void handleKey(int key, bool pressed);
		virtual void handleMouseClick(int button, bool pressed);
		virtual void handleMouseMove(float mx, float my);


	private:
		WindowInterface* window_;
		RHI::IRHIModule* rhiModule_;
		RHI::IDynamicRHI* dynamicRHI_;
		RHI::IDevice* device_;
		CameraPositioner_FirstPerson positioner_;
		TestCamera camera_;
		FramesPerSecondCounter fpsCounter_;
	};
}
