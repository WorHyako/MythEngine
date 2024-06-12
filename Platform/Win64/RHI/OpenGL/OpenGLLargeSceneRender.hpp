#pragma once

#include <RHI/OpenGL/Framework/GLFWApp.hpp>
#include <RHI/OpenGL/OpenGLBaseRender.hpp>

class CameraPositioner_FirstPerson;
class TestCamera;

class OpenGLLargeSceneRender : public OpenGLBaseRender
{
public:
	OpenGLLargeSceneRender(GLApp* app);

	virtual int draw() override;

private:
	CameraPositioner_FirstPerson* positioner_;
	TestCamera* testCamera_;
};