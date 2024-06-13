#pragma once
#include <RHI/OpenGL/OpenGLBaseRender.hpp>

class OpenGLRender : public OpenGLBaseRender
{
public:
    OpenGLRender(GLApp* app);
    ~OpenGLRender() override = default;

    virtual int draw() override;
};