#pragma once

#include <cassert>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>

#ifndef PROJECT_ROOT_DIR
#define PROJECT_ROOT_DIR ""
#endif

#ifndef PLATFORM_DIR
#define PLATFORM_DIR ""
#endif

class FilesystemUtilities
{
public:
    static std::string ReadFile(const std::string& FilePath)
    {
        std::ifstream InputFile(FilePath);
        if (!InputFile.is_open()) {
            std::cerr << "Could not open the file - '" << FilePath << "'" << std::endl;
            return std::string();
        }
        return std::string((std::istreambuf_iterator<char>(InputFile)), std::istreambuf_iterator<char>());
    }
    
    static std::string GetRootDir()
    {
        assert(std::strlen(PROJECT_ROOT_DIR) > 0);
        return std::string(PROJECT_ROOT_DIR);
    }
    
    static std::string GetPlatformDir()
    {
        assert(std::strlen(PLATFORM_DIR) > 0);
        return std::string(PLATFORM_DIR);
    }
    
    static std::string GetShadersDir()
    {
        return GetPlatformDir() + std::string("/Shaders/");
    }

    static std::string GetResourcesDir()
    {
        return GetRootDir() + std::string("/Resources/");
    }
};
