#pragma once

#include <System/ApplicationInterface.hpp>
#include <RHI/RHICommon/RHIModuleWrapper.hpp>
#include <Camera/TestCamera.hpp>
#include "Utils/UtilsFPS.hpp"

// In order to define a function called CreateWindow, the Windows macro needs to
// be undefined.
#if defined(CreateWindow)
#undef CreateWindow
#endif

class IRHIModule;

namespace mythSystem
{
	class Application : public ApplicationInterface
	{
	public:
		Application();
		virtual ~Application();

		virtual void mainLoop() override;
		virtual void update(float deltaSeconds) override;
		virtual UniquePtr<WindowInterface> createWindow() override;
		virtual UniquePtr<RendererInterface> createRenderer() override;

		UniquePtr<WindowInterface> createWindowGLFW();

		virtual void handleKey(int key, bool pressed);
		virtual void handleMouseClick(int button, bool pressed);
		virtual void handleMouseMove(float mx, float my);


	private:
		UniquePtr<WindowInterface> m_Window;
		UniquePtr<RendererInterface> m_Renderer;
		RHI::IRHIModule* m_RhiModule;
		RHI::IDynamicRHI* m_DynamicRHI;
		RHI::IDevice* m_Device;
		RHI::GraphicsAPI m_GraphicsApi;
		CameraPositioner_FirstPerson m_Positioner;
		TestCamera m_Camera;
		FramesPerSecondCounter m_FpsCounter;

		struct MouseState
		{
			glm::vec2 pos = glm::vec2(0.0f);
			bool pressedLeft = false;
		} m_MouseState;
	};
}
