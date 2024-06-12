#pragma once

#include <EngineTypes.hpp>
#include <System/WindowInterface.hpp>


namespace mythSystem
{

    class ApplicationInterface
    {
    public:
        ApplicationInterface(){};
        virtual ~ApplicationInterface(){};

        virtual void OnApplicationStarted() = 0;
        virtual void Exit(int ExitCode = 0) = 0;

    protected:
        virtual UniquePtr<WindowInterface> CreateWindow() = 0;
    };

};// namespace mythSystem
