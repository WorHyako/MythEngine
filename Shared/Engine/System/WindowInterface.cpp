#include "WindowInterface.hpp"

#include "EngineConfiguration.hpp"
#include "Filesystem/Config.hpp"
#include "Filesystem/FilesystemUtilities.hpp"

namespace mythSystem
{
    WindowInterface::WindowInterface()
    {
        Config Config(FilesystemUtilities::GetPlatformDir() + "/Config.ini");

        Config.ReadValueSafety("Windows", "Width", WindowParams.Width);
        Config.ReadValueSafety("Windows", "Height", WindowParams.Height);
        Config.ReadValueSafety("Windows", "WindowTitle", WindowParams.WindowTitle);
        Config.ReadValueSafety("Windows", "GraphicsInterface", WindowParams.GraphicsInterface);
    }

    WindowInterface::~WindowInterface()
    {
    }

    bool WindowInterface::Initialize()
    {
        return true;
    }

    int WindowInterface::Run()
    {
        return 1;
    }
}