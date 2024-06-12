#pragma once

#include <RHI/OpenGL/OpenGLBaseRender.hpp>

class OpenGLStudyRender : public OpenGLBaseRender
{
public:
	OpenGLStudyRender(GLApp* app);

	virtual int draw() override;
};