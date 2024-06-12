#include <RHI/Vulkan/VulkanBaseRender.hpp>
#include <GLFW/glfw3.h>

VulkanBaseRender::VulkanBaseRender(const uint32_t screenWidth, uint32_t screenHeight)
	: kScreenWidth(screenWidth)
	, kScreenHeight(screenHeight)
{
	
}

void VulkanBaseRender::SetWindow(GLFWwindow* window)
{
	window_ = window;
}

GLFWwindow* VulkanBaseRender::GetWindow()
{
	return window_;
}