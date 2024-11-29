#include <System/Application.hpp>
#include <System/WindowGLFW.hpp>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "GLFW/glfw3.h"
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;

namespace mythSystem
{
    Application::Application()
        : positioner_(glm::vec3(0.0f, 5.0f, 10.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, -1.0f, 0.0f))
        , camera_(positioner_)
    {
        rhiModule_ = RHI::InitializeModuleRHI(RHI::GraphicsAPI::VULKAN);
        CreateWindowGLFW();
        window_->setWindowUserPointer(this);
        window_->assignCallbacks();
        if (rhiModule_)
        {
            dynamicRHI_ = rhiModule_->createRHI();
            if (dynamicRHI_)
            {
                device_ = dynamicRHI_->getDevice();
            }

        }
    }

    Application::~Application()
    {

    }

    UniquePtr<WindowInterface> Application::CreateWindow()
    {
	    
    }

    void Application::CreateWindowGLFW()
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

        GLFWWindow* vulkanWindow = new GLFWWindow(detectResolution(SCREEN_WIDTH, SCREEN_HEIGHT));
        if (!vulkanWindow->getWindow())
        {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        window_ = vulkanWindow;
    }

    void Application::handleKey(int key, bool pressed)
    {

    }

    void Application::handleMouseClick(int button, bool pressed)
    {
        {
            if (button == GLFW_MOUSE_BUTTON_LEFT)
                mouseState_.pressedLeft = pressed;
        }
    }

    void Application::handleMouseMove(float mx, float my)
    {
        {
            mouseState_.pos.x = mx;
            mouseState_.pos.y = my;
        }
    }
}
