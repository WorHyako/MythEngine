#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <EngineConfiguration.hpp>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

class GLApp
{
public:
    GLApp(const WindowParameters* WindowParams = nullptr)
    {
        glfwSetErrorCallback(
            [](int error, const char* description)
            {
                fprintf(stderr, "Error: %s\n", description);
            }
        );

        if (!glfwInit())
            exit(EXIT_FAILURE);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
        glfwWindowHint(GLFW_SAMPLES, 4);

        int width, height;
        if(WindowParams)
        {
            width = WindowParams->Width;
            height = WindowParams->Height;
        }
        else
        {
            const GLFWvidmode* info = glfwGetVideoMode(glfwGetPrimaryMonitor());
            width = info->width;
            height = info->height;
        }

        window_ = glfwCreateWindow(width, height, "Simple example", nullptr, nullptr);

        if(!window_)
        {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        glfwMakeContextCurrent(window_);
        if(!gladLoadGL(glfwGetProcAddress))
        {
            printf("Unable to load glad\n");
            exit(255);
        }
        glfwSwapInterval(0);


    }
    ~GLApp()
    {
        glfwDestroyWindow(window_);
        glfwTerminate();
    }
    GLFWwindow* getWindow() const { return window_; }
    float getDeltaSeconds() const { return deltaSeconds_; }
    void swapBuffers()
    {
        glfwSwapBuffers(window_);
        glfwPollEvents();

        assert(glGetError() == GL_NO_ERROR);

        const double newTimeStamp = glfwGetTime();
        deltaSeconds_ = static_cast<float>(newTimeStamp - timeStamp_);
        timeStamp_ = newTimeStamp;
    }

private:
    GLFWwindow* window_ = nullptr;
    double timeStamp_ = glfwGetTime();
    float deltaSeconds_ = 0;
};