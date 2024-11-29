#pragma once
#include <System/WindowInterface.hpp>
#include <GLFW/glfw3.h>

namespace mythSystem
{
	Resolution detectResolution(int width, int height);

	struct GLFWWindow : public WindowInterface
	{
		GLFWWindow(Resolution resolution);

		~GLFWWindow() override
		{
			window_ = nullptr;
		}

		GLFWwindow* getWindow()
		{
			return window_;
		}

		virtual void setWindowUserPointer(void* pointer) override;
		virtual void assignCallbacks() override;

		virtual void handleKey(int key, bool pressed);
		virtual void handleMouseClick(int button, bool pressed);
		virtual void handleMouseMove(float mx, float my);

	private:
		GLFWwindow* window_;
	};
}
