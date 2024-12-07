#pragma once

#include <EngineTypes.hpp>
#include <System/WindowInterface.hpp>


namespace mythSystem
{
    class RendererInterface;

    class ApplicationInterface
    {
    public:
        ApplicationInterface(){};
        virtual ~ApplicationInterface(){};

        virtual void OnApplicationStarted() = 0;
        virtual void Exit(int ExitCode = 0) = 0;
        virtual void mainLoop() = 0;
        virtual void update(float deltaSeconds) = 0;

    protected:
        virtual void createWindow() = 0;
        virtual void createRenderer() = 0;
    };

};// namespace mythSystem
