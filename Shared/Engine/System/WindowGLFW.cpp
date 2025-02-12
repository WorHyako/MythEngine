#include <System/WindowGLFW.hpp>
#include <imgui.h>

#include "Application.hpp"

namespace mythSystem
{
    Resolution detectResolution(int width, int height)
    {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const int code = glfwGetError(nullptr);

        if (code != 0)
        {
            printf("Monitor: %p; error = %x / %d\n", monitor, code, code);
            exit(255);
        }

        const GLFWvidmode* info = glfwGetVideoMode(monitor);

        const uint32_t windowW = width > 0 ? width : (uint32_t)(info->width * width / -100);
        const uint32_t windowH = height > 0 ? height : (uint32_t)(info->height * height / -100);

        return Resolution{ windowW, windowH };
    }

    GLFWWindow::GLFWWindow(Resolution resolution)
	    : WindowInterface(resolution)
    {
        m_Window = glfwCreateWindow(resolution.width, resolution.height, "MythEngine", nullptr, nullptr);
    }

    void GLFWWindow::setWindowUserPointer(void* pointer)
    {
        if (m_Window)
        {
            glfwSetWindowUserPointer(m_Window, pointer);
        }
    }

    void GLFWWindow::assignCallbacks()
    {
        glfwSetCursorPosCallback(
            m_Window,
            [](GLFWwindow* window, double x, double y)
            {
                //ImGui::GetIO().MousePos = ImVec2((float)x, (float)y);
                int width, height;
                glfwGetFramebufferSize(window, &width, &height);

                const float mx = static_cast<float>(x / width);
                const float my = static_cast<float>(y / height);

                if (void* ptr = glfwGetWindowUserPointer(window))
                {
                    reinterpret_cast<Application*>(ptr)->handleMouseMove(mx, my);
                }
            }
        );

        glfwSetMouseButtonCallback(
            m_Window,
            [](GLFWwindow* window, int button, int action, int mods)
            {
                //auto& io = ImGui::GetIO();
                const int idx = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 1;
                /*io.MouseDown[idx] = */action == GLFW_PRESS;

                if (void* ptr = glfwGetWindowUserPointer(window))
                {
                    reinterpret_cast<Application*>(ptr)->handleMouseClick(button, action == GLFW_PRESS);
                }
            }
        );

        glfwSetKeyCallback(
            m_Window,
            [](GLFWwindow* window, int key, int scancode, int action, int mods)
            {
                const bool pressed = action != GLFW_RELEASE;
                if (key == GLFW_KEY_ESCAPE && pressed)
                    glfwSetWindowShouldClose(window, GLFW_TRUE);

                if (void* ptr = glfwGetWindowUserPointer(window))
                {
                    reinterpret_cast<Application*>(ptr)->handleKey(key, pressed);
                }
            }
        );
    }

    void GLFWWindow::getFramebufferResolution(Resolution& resolution)
    {
        int width, height;
        glfwGetFramebufferSize(m_Window, &width, &height);
        resolution.width = width;
        resolution.height = height;
    }

    bool GLFWWindow::Initialize()
    {
        return true;
    }

    void GLFWWindow::createWindowSurface()
    {

    }

    uint32_t GLFWWindow::getRequiredExtension(std::vector<const char*>& extensions)
    {
        uint32_t extensionsCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionsCount);
        for (uint32_t i = 0; i < extensionsCount; i++) {
            extensions.push_back(glfwExtensions[i]);
        }
        return extensionsCount;
    }

    bool GLFWWindow::IsClosed()
    {
        return glfwWindowShouldClose(m_Window);
    }

    void GLFWWindow::handleMouseMove(float mx, float my)
    {
        
    }

    void GLFWWindow::handleMouseClick(int button, bool pressed)
    {
	    
    }

    void GLFWWindow::handleKey(int key, bool pressed)
    {
        
    }

}
