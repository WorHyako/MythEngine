#pragma once

#include <EngineTypes.hpp>

#include <EngineConfiguration.hpp>

namespace mythSystem
{

    class WindowInterface
    {
    public:
        WindowInterface();
        virtual ~WindowInterface();

        virtual bool Initialize();

        virtual int Run();

    protected:
        WindowParameters WindowParams;
    };

};// namespace mythSystem
