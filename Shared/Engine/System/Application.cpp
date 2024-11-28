#include <System/Application.hpp>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;

Application::Application()
	: positioner_(glm::vec3(0.0f, 5.0f, 10.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, -1.0f, 0.0f))
	, camera_(positioner_)
{
	rhiModule_ = RHI::InitializeModuleRHI(RHI::GraphicsAPI::VULKAN);
    CreateWindowGLFW();
    window_->setWindowUserPointer(this);
    window_->assignCallbacks();
}

Application::~Application()
{
	
}

void Application::CreateWindowGLFW()
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    RHI::Vulkan::GLFWWindow* vulkanWindow = new RHI::Vulkan::GLFWWindow(RHI::Vulkan::detectResolution(SCREEN_WIDTH, SCREEN_HEIGHT));
    if (!vulkanWindow->getWindow())
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    window_ = vulkanWindow;
}
