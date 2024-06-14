#pragma once

#include <RHI/OpenGL/OpenGLBaseRender.hpp>

class CameraPositioner_FirstPerson;
class TestCamera;

class OpenGLSSAORender : public OpenGLBaseRender
{
public:
	OpenGLSSAORender(GLApp* app);
	~OpenGLSSAORender() override = default;

	virtual int draw() override;

private:
	CameraPositioner_FirstPerson* positioner_;
	TestCamera* testCamera_;
};