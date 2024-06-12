#pragma once

#include <RHI/OpenGL/OpenGLBaseRender.hpp>

#include <glm/glm.hpp>

class CameraPositioner_FirstPerson;
class TestCamera;

class OpenGLCullingGPURender : public OpenGLBaseRender
{
public:
	OpenGLCullingGPURender(GLApp* app);

	virtual int draw() override;

private:
	CameraPositioner_FirstPerson* positioner_;
	TestCamera* testCamera_;

	bool enableGPUCulling = true;
	glm::mat4 cullingView;
};