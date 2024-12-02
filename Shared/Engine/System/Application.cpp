#include <System/Application.hpp>
#include <System/WindowGLFW.hpp>
#include <Renderer/RendererInterface.hpp>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "GLFW/glfw3.h"
#include "RHI/Vulkan/VulkanSceneRenderer.hpp"
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
        m_GraphicsApi = RHI::GraphicsAPI::VULKAN;
        m_RhiModule = RHI::InitializeModuleRHI(m_GraphicsApi);
        m_Window = createWindow();
        m_Window->setWindowUserPointer(this);
        m_Window->assignCallbacks();

        if (m_RhiModule)
        {
            m_DynamicRHI = m_RhiModule->createRHI();

            if(m_GraphicsApi == RHI::GraphicsAPI::VULKAN)
            {
                if (RHI::Vulkan::VulkanDynamicRHI* VulkanDynamicRHI = dynamic_cast<RHI::Vulkan::VulkanDynamicRHI*>(m_DynamicRHI))
                {
                    if (GLFWWindow* windowGLFW = dynamic_cast<GLFWWindow*>(m_Window.get()))
                    {
                        VulkanDynamicRHI->createWindowSurface(windowGLFW->getWindow());
                    }

                    RHI::Vulkan::DeviceDesc desc = {};
                    desc.useGraphicsQueue = true;
                    desc.useComputeQueue = true;
                    desc.ctxExtensions = &RHI::Vulkan::VulkanDynamicRHI::initializeContextExtensions();
                    desc.ctxFeatures = &RHI::Vulkan::VulkanDynamicRHI::initializeContextFeatures();
                    m_Device = VulkanDynamicRHI->createDevice(desc);
                }
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
        m_Renderer = std::make_unique<VulkanSceneRenderer>();
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

            m_Renderer->renderScene();
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
