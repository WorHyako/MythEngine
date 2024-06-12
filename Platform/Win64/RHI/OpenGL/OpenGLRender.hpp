#pragma once
#include <RHI/OpenGL/OpenGLBaseRender.hpp>

class OpenGLRender : public OpenGLBaseRender
{
public:
    OpenGLRender(GLApp* app);
    ~OpenGLRender(){}

    virtual int draw() override;
};