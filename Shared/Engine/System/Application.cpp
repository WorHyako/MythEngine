#include <System/Application.hpp>
#include <System/WindowGLFW.hpp>
#include <Renderer/RendererInterface.hpp>

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
        : m_Positioner(glm::vec3(0.0f, 5.0f, 10.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, -1.0f, 0.0f))
        , m_Camera(m_Positioner)
    {
        m_RhiModule = RHI::InitializeModuleRHI(RHI::GraphicsAPI::VULKAN);
        m_Window = createWindow();
        m_Window->setWindowUserPointer(this);
        m_Window->assignCallbacks();
        if (m_RhiModule)
        {
            m_DynamicRHI = m_RhiModule->createRHI();
            if (m_DynamicRHI)
            {
                m_Device = m_DynamicRHI->getDevice();
            }
        }


    }

    Application::~Application()
    {
        delete m_RhiModule;
        delete m_DynamicRHI;
        delete m_Device;
        delete m_Window;
    }

    UniquePtr<WindowInterface> Application::createWindow()
    {
        return createWindowGLFW();
    }

    UniquePtr<RendererInterface> Application::createRenderer()
    {
        
    }

    UniquePtr<WindowInterface> Application::createWindowGLFW()
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

        GLFWWindow* vulkanWindow = new GLFWWindow(detectResolution(SCREEN_WIDTH, SCREEN_HEIGHT));
        if (!vulkanWindow->getWindow())
        {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
    }

    void Application::mainLoop()
    {
        double timeStamp = glfwGetTime();
        float deltaSeconds = 0.0f;

        do
        {
            update(deltaSeconds);

            const double newTimeStamp = glfwGetTime();
            deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
            timeStamp = newTimeStamp;

            m_FpsCounter.tick(deltaSeconds);

            bool frameRendered = drawFrame(ctx_.vkDev,
                [this](uint32_t img) {this->updateBuffers(img); },
                [this](auto cmd, auto img) {ctx_.composeFrame(cmd, img); }
            );

            m_FpsCounter.tick(deltaSeconds, frameRendered);

            glfwPollEvents();

        } while (!m_Window->IsClosed());
    }

    void Application::handleKey(int key, bool pressed)
    {
        if (key == GLFW_KEY_W)
            m_Positioner.movement_.forward_ = pressed;
        if (key == GLFW_KEY_S)
            m_Positioner.movement_.backward_ = pressed;
        if (key == GLFW_KEY_A)
            m_Positioner.movement_.left_ = pressed;
        if (key == GLFW_KEY_D)
            m_Positioner.movement_.right_ = pressed;
        if (key == GLFW_KEY_E)
            m_Positioner.movement_.up_ = pressed;
        if (key == GLFW_KEY_Q)
            m_Positioner.movement_.down_ = pressed;
    }

    void Application::handleMouseClick(int button, bool pressed)
    {
        {
            if (button == GLFW_MOUSE_BUTTON_LEFT)
                m_MouseState.pressedLeft = pressed;
        }
    }

    void Application::handleMouseMove(float mx, float my)
    {
        {
            m_MouseState.pos.x = mx;
            m_MouseState.pos.y = my;
        }
    }
}
