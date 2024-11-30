#pragma once
#include <System/WindowInterface.hpp>
#include <GLFW/glfw3.h>

namespace mythSystem
{
	Resolution detectResolution(int width, int height);

	struct GLFWWindow : public WindowInterface
	{
		GLFWWindow(Resolution resolution);

		virtual ~GLFWWindow() override
		{
			m_Window = nullptr;
		}

		virtual bool IsClosed() override;

		GLFWwindow* getWindow()
		{
			return m_Window;
		}

		virtual void setWindowUserPointer(void* pointer) override;
		virtual void assignCallbacks() override;

		virtual void handleKey(int key, bool pressed);
		virtual void handleMouseClick(int button, bool pressed);
		virtual void handleMouseMove(float mx, float my);

	private:
		GLFWwindow* m_Window;
	};
}
