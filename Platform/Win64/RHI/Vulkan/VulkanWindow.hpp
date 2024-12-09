#pragma once

#include "System/WindowInterface.hpp"

struct GLFWwindow;
class VulkanRender;
class VulkanBaseRender;
class CameraApp;

class VulkanWindow : public mythSystem::WindowInterface
{
public:
	VulkanWindow();
	~VulkanWindow();

	virtual int Run() override;

	void InitializeCallbacks();

	virtual void getFramebufferResolution(mythSystem::Resolution& resolution){};

	virtual void setWindowUserPointer(void* pointer){};
	virtual void assignCallbacks(){};

	virtual void handleKey(int key, bool pressed) {};
	virtual void handleMouseClick(int button, bool pressed) {};
	virtual void handleMouseMove(float mx, float my) {};

private:
	GLFWwindow* window_ = nullptr;
	VulkanRender* render_ = nullptr;
	VulkanBaseRender* vulkanRender_ = nullptr;
	CameraApp* sceneRender_ = nullptr;
};