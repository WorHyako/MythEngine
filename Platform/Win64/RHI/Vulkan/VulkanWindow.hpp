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

private:
	GLFWwindow* window_ = nullptr;
	VulkanRender* render_ = nullptr;
	VulkanBaseRender* vulkanRender_ = nullptr;
	CameraApp* sceneRender_ = nullptr;
};