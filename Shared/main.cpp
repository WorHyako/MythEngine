#include <memory>

#include <System/WindowInterface.hpp>
#include <Filesystem/FilesystemUtilities.hpp>
#include <Utils/RedirectToConsole.hpp>
#include <Utils/UtilsFPS.hpp>

#ifdef _WIN64
#include <RHI/DX12/D3D12Window.hpp>
#include <RHI/OpenGL/OpenGLWindow.hpp>
#include <RHI/Vulkan/VulkanWindow.hpp>

int __stdcall WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
{
    mythSystem::WindowInterface* Window;

    RedirectIOToConsole();

	Window = new OpenGLWindow();
    //Window = new VulkanWindow();

    Window->Initialize();
    Window->Run();
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
