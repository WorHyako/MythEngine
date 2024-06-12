#pragma once
#include <RHI/Vulkan/UtilsVulkan.hpp>
#include <memory>

struct GLFWwindow;
class GLFWUserInput;

class VulkanBaseRender
{
public:
	VulkanBaseRender(const uint32_t screenWidth, uint32_t screenHeight);

	virtual bool initVulkan() = 0;
	virtual void terminateVulkan() = 0;
	virtual int draw() = 0;
	virtual void recreateSwapchain() {}
	virtual void cleanupSwapchain() {}

public:
	void SetWindow(GLFWwindow* window);
	GLFWwindow* GetWindow();

protected:
	VulkanInstance vk_;
	VulkanRenderDevice vkDev;
	GLFWwindow* window_;
	GLFWUserInput* input_;
	const uint32_t kScreenWidth;
	const uint32_t kScreenHeight;
};