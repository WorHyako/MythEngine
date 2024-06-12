#pragma once

#include <RHI/OpenGL/OpenGLBaseRender.hpp>

class CameraPositioner_FirstPerson;
class TestCamera;

class OpenGLShadowMappingRender : public OpenGLBaseRender
{
public:
	OpenGLShadowMappingRender(GLApp* app);

	virtual int draw() override;

private:
	CameraPositioner_FirstPerson* positioner_;
	TestCamera* testCamera_;
};