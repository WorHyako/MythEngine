#pragma once
#include <System/WindowInterface.hpp>
#include <GLFW/glfw3.h>

namespace mythSystem
{
	Resolution detectResolution(int width, int height);

	class GLFWWindow : public WindowInterface
	{
	public:
		GLFWWindow(Resolution resolution);

		virtual ~GLFWWindow() override
		{
			m_Window = nullptr;
		}

		bool Initialize() override;
		virtual bool IsClosed() override;

		GLFWwindow* getWindow()
		{
			return m_Window;
		}

		void createWindowSurface();
		virtual void setWindowUserPointer(void* pointer) override;
		virtual void assignCallbacks() override;
		virtual void getFramebufferResolution(Resolution& resolution) override;
		virtual uint32_t getRequiredExtension(std::vector<const char*>& extensions) override;

		virtual void handleKey(int key, bool pressed);
		virtual void handleMouseClick(int button, bool pressed);
		virtual void handleMouseMove(float mx, float my);

	private:
		GLFWwindow* m_Window;
	};
}
