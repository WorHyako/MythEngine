#include <RHI/Vulkan/VulkanWindow.hpp>

#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <RHI/Vulkan/VulkanRender.hpp>
#include <RHI/Vulkan/VulkanStudyRender.hpp>
#include <RHI/Vulkan/VulkanNonuniformIndexingRender.hpp>
#include <RHI/Vulkan/VulkanComputeTextureRender.hpp>
#include <RHI/Vulkan/VulkanComputeMeshRender.hpp>
#include <RHI/Vulkan/VulkanBRDFLUTRender.hpp>
#include <RHI/Vulkan/Vulkan_GLTF_PBRRender.hpp>
#include <RHI/Vulkan/VulkanLargeSceneRender.hpp>
#include <RHI/Vulkan/VulkanSceneGraphRender.hpp>
#include <RHI/Vulkan/VulkanShadowMappingRender.hpp>
#include <RHI/Vulkan/VulkanSSAORender.hpp>
#include <RHI/Vulkan/VulkanHDRRender.hpp>
#include <RHI/Vulkan/VulkanPhysicsRender.hpp>
#include <RHI/Vulkan/VulkanAtomicTestRender.hpp>
#include <RHI/Vulkan/VulkanSceneCompositionRender.hpp>
#include <UserInput/GLFW/GLFWUserInput.hpp>

VulkanWindow::VulkanWindow()
	:WindowInterface()
{
	//render_ = new VulkanRender();

	/*vulkanRender_ = new VulkanComputeMeshRender(1600, 900);
	window_ = vulkanRender_->GetWindow();

    GLFWUserInput* userInput = &GLFWUserInput::GetInput();
    glfwSetWindowUserPointer(window_, userInput);
    InitializeCallbacks();*/

	//generateMeshFile();
	//generateData();
	sceneRender_ = new SceneCompositionApp();

}

VulkanWindow::~VulkanWindow()
{

}

int VulkanWindow::Run()
{
	/*if (render_)
	{
		render_->Draw();
	}*/
    static bool doOnce = true;
#if 0
    if (doOnce)
    {
		if (vulkanRender_)
		{
			vulkanRender_->draw();
		}

        doOnce = false;
    }
#else
	if (doOnce)
	{
		if (sceneRender_)
		{
			sceneRender_->mainLoop();
		}

		doOnce = false;
	}
#endif
    
	return 0;
}

void VulkanWindow::InitializeCallbacks()
{
	auto keyboard_callback_func = [](GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		static_cast<GLFWUserInput*>(glfwGetWindowUserPointer(window))->keyboard_callback(window, key, scancode, action, mods);
	};
	glfwSetKeyCallback(window_, keyboard_callback_func);

	auto framebuffer_size_callback_func = [](GLFWwindow* window, int width, int height)
	{
		static_cast<GLFWUserInput*>(glfwGetWindowUserPointer(window))->framebuffer_size_callback(window, width, height);
	};
	glfwSetFramebufferSizeCallback(window_, framebuffer_size_callback_func);

	auto mouse_callback_func = [](GLFWwindow* window, double xposIn, double yposIn)
	{
		if(ImGui::GetCurrentContext())
		{
			ImGui::GetIO().MousePos = ImVec2(xposIn, yposIn);
		}
		static_cast<GLFWUserInput*>(glfwGetWindowUserPointer(window))->mouse_callback(window, xposIn, yposIn);
	};
	glfwSetCursorPosCallback(window_, mouse_callback_func);

	auto scroll_callback_func = [](GLFWwindow* window, double xoffset, double yoffset)
	{
		static_cast<GLFWUserInput*>(glfwGetWindowUserPointer(window))->scroll_callback(window, xoffset, yoffset);
	};
	glfwSetScrollCallback(window_, scroll_callback_func);

	glfwSetMouseButtonCallback(window_, [](auto* window, int button, int action, int mods)
	{
		bool ioWantCaptureMouse = false;

		if(ImGui::GetCurrentContext())
		{
			auto& io = ImGui::GetIO();
			int idx = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 1;
			io.MouseDown[idx] = action == GLFW_PRESS;
			ioWantCaptureMouse = io.WantCaptureMouse;
		}

		static_cast<GLFWUserInput*>(glfwGetWindowUserPointer(window))->mousebutton_callback(window, button, action, mods, ioWantCaptureMouse);
	});
}