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

private:
	GLFWwindow* window = nullptr;
	OpenGLBaseRender* render = nullptr;
	ImGuiIO* io = nullptr;
	GLApp* app = nullptr;
};