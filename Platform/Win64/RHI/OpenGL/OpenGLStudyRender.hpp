#pragma once

#include <RHI/OpenGL/OpenGLBaseRender.hpp>

class OpenGLStudyRender : public OpenGLBaseRender
{
public:
	OpenGLStudyRender(GLApp* app);
	~OpenGLStudyRender() override = default;

	virtual int draw() override;
};