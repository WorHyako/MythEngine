#pragma once

#include <RHI/OpenGL/OpenGLBaseRender.hpp>

class CameraPositioner_FirstPerson;
class TestCamera;

class OpenGLCullingCPURender : public OpenGLBaseRender
{
public:
	OpenGLCullingCPURender(GLApp* app);

	virtual int draw() override;

private:
	CameraPositioner_FirstPerson* positioner_;
	TestCamera* testCamera_;
};