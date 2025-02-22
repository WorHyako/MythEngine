#pragma once

#include "System/WindowInterface.hpp"

struct GLFWwindow;
class GLApp;
class OpenGLBaseRender;

struct ImGuiIO;

class OpenGLWindow : public mythSystem::WindowInterface
{
public:
	OpenGLWindow();
	~OpenGLWindow();

	virtual int Run() override;

	void InitializeCallbacks();
	ImGuiIO* GetIO() const { return io; }

	virtual void getFramebufferResolution(mythSystem::Resolution& resolution) {};

	virtual void setWindowUserPointer(void* pointer) {};
	virtual void assignCallbacks() {};
	virtual void createWindowSurface() override {};
	virtual uint32_t getRequiredExtension(std::vector<const char*>& extensions) override
	{
		return 0;
	};

	virtual void handleKey(int key, bool pressed) {};
	virtual void handleMouseClick(int button, bool pressed) {};
	virtual void handleMouseMove(float mx, float my) {};

private:
	GLFWwindow* window = nullptr;
	OpenGLBaseRender* render = nullptr;
	ImGuiIO* io = nullptr;
	GLApp* app = nullptr;
};