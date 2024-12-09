#pragma once

#include <EngineTypes.hpp>

#include <EngineConfiguration.hpp>

namespace mythSystem
{
    struct Resolution
    {
        uint32_t width = 0;
        uint32_t height = 0;
    };

    class WindowInterface
    {
    public:
        WindowInterface() = default;
        WindowInterface(Resolution resolution);
        virtual ~WindowInterface();

        virtual bool Initialize();
        virtual bool IsClosed();

        virtual int Run();

        virtual Resolution& getResolution()
        {
            return resolution;
        }

        virtual void getFramebufferResolution(Resolution& resolution) = 0;

        virtual void setWindowUserPointer(void* pointer) = 0;
        virtual void assignCallbacks() = 0;

        virtual void handleKey(int key, bool pressed) = 0;
        virtual void handleMouseClick(int button, bool pressed) = 0;
        virtual void handleMouseMove(float mx, float my) = 0;

    protected:
        WindowParameters WindowParams;
        Resolution resolution;
    };

};// namespace mythSystem
