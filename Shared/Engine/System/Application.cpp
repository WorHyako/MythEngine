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

mythSystem::Application* gs_pApplication = nullptr;

namespace mythSystem
{
    Application::Application()
        : m_Positioner(glm::vec3(0.0f, 5.0f, 10.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, -1.0f, 0.0f))
        , m_Camera(m_Positioner)
    {
        gs_pApplication = this;

        m_GraphicsAPI = RHI::GraphicsAPI::VULKAN;
        m_RhiModule = RHI::InitializeModuleRHI(m_GraphicsAPI);

        if (!glfwInit())
        {
            printf("Cannot initialize GLFW\n");
            exit(EXIT_FAILURE);
        }

        if (!glfwVulkanSupported())
        {
            printf("GLFW don't support Vulkan\n");
            exit(EXIT_FAILURE);
        }

        createWindow();
        createDynamicRHI();
        m_Window->setWindowUserPointer(this);
        m_Window->assignCallbacks();
        createRenderer();
    }

    Application::~Application()
    {
        delete m_RhiModule;
        delete m_DynamicRHI;
    	m_Device = nullptr;

    	m_Window.release();
        m_Window = nullptr;
    }

    void Application::createWindow()
    {
        createWindowGLFW();
    }

    void Application::createRenderer()
    {
        m_Renderer = std::make_unique<VulkanSceneRenderer>(m_DynamicRHI);
    }

    void Application::createDynamicRHI()
    {
        if (m_RhiModule)
        {
            RHI::DeviceParams deviceParams = {};
            deviceParams.useGraphicsQueue = true;
            deviceParams.useComputeQueue = true;
            deviceParams.useTransferQueue = true;
            deviceParams.usePresentQueue = true;
            Resolution resolution;
            m_Window.get()->getFramebufferResolution(resolution);
            deviceParams.backBufferWidth = resolution.width;
            deviceParams.backBufferHeight = resolution.height;
            m_Window->getRequiredExtension(deviceParams.requiredVulkanInstanceExtensions);
            deviceParams.vSyncEnabled = false;

            m_DynamicRHI = m_RhiModule->createRHI(deviceParams);

            if (m_GraphicsAPI == RHI::GraphicsAPI::VULKAN)
            {
                if (RHI::Vulkan::VulkanDynamicRHI* VulkanDynamicRHI = dynamic_cast<RHI::Vulkan::VulkanDynamicRHI*>(m_DynamicRHI))
                {
                    if (GLFWWindow* windowGLFW = dynamic_cast<GLFWWindow*>(m_Window.get()))
                    {
                        VkSurfaceKHR surface;
                        glfwCreateWindowSurface(VulkanDynamicRHI->getVulkanInstance().instance, windowGLFW->getWindow(), nullptr, &surface);
                        VulkanDynamicRHI->setWindowSurface(surface);
                    }
                }
            }

            m_DynamicRHI->CreateDevice();
        }
    }

    void Application::createWindowGLFW()
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

        m_Window = std::make_unique<GLFWWindow>(GLFWWindow(detectResolution(SCREEN_WIDTH, SCREEN_HEIGHT)));
        if (!m_Window.get())
        {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
    }

    void Application::update(float deltaSeconds)
    {
	    
    }

    void Application::mainLoop()
    {
        m_Renderer->initializeRender();

        double timeStamp = glfwGetTime();
        float deltaSeconds = 0.0f;

        do
        {
            update(deltaSeconds);

            const double newTimeStamp = glfwGetTime();
            deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
            timeStamp = newTimeStamp;

            m_FpsCounter.tick(deltaSeconds);

            m_DynamicRHI->BeginFrame();
            bool frameRendered = m_Renderer->renderScene();
            m_DynamicRHI->Present();

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
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            m_MouseState.pressedLeft = pressed;
    }

    void Application::handleMouseMove(float mx, float my)
    {
        m_MouseState.pos.x = mx;
        m_MouseState.pos.y = my;
    }

    Application& Application::Get()
    {
        assert(gs_pApplication != nullptr);
        return *gs_pApplication;
    }
}
