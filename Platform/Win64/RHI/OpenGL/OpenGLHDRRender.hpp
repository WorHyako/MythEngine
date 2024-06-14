#pragma once

#include <RHI/OpenGL/Framework/GLFWApp.hpp>
#include <RHI/OpenGL/OpenGLBaseRender.hpp>

class CameraPositioner_FirstPerson;
class TestCamera;

class OpenGLHDRRender : public OpenGLBaseRender
{
public:
	OpenGLHDRRender(GLApp* app);
	~OpenGLHDRRender() override = default;

	virtual int draw() override;

private:
	CameraPositioner_FirstPerson* positioner_;
	TestCamera* testCamera_;
};