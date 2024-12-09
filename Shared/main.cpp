#include <memory>

#include <System/WindowInterface.hpp>
#include <System/Application.hpp>
#include <Filesystem/FilesystemUtilities.hpp>
#include <Utils/RedirectToConsole.hpp>
#include <Utils/UtilsFPS.hpp>

#ifdef _WIN64
#include <RHI/DX12/D3D12Window.hpp>
#include <RHI/OpenGL/OpenGLWindow.hpp>
#include <RHI/Vulkan/VulkanWindow.hpp>

#define USE_VULKAN_RHI 0

int __stdcall WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
{
#if USE_VULKAN_RHI

#else
    mythSystem::WindowInterface* Window;

    RedirectIOToConsole();

    mythSystem::Application* application;
    application = new mythSystem::Application();
    application->mainLoop();

    //Window = new OpenGLWindow();
    Window = new VulkanWindow();

    Window->Initialize();
    Window->Run();
#endif
};
#endif


#ifdef __APPLE__
#include "RHI/Vulkan/VulkanWindow.hpp"

int main(int argc, char* argv[])
{
    mythSystem::WindowInterface* Window = new VulkanWindow();

    Window->Initialize();
    Window->Run();
    
    return 0;
}
#endif
