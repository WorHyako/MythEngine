#pragma once

#include <RHI/OpenGL/OpenGLBaseRender.hpp>

class CameraPositioner_FirstPerson;
class TestCamera;

class OpenGLOITransparencyRender : public OpenGLBaseRender
{
public:
	OpenGLOITransparencyRender(GLApp* app);

	virtual int draw() override;

private:
	CameraPositioner_FirstPerson* positioner_;
	TestCamera* testCamera_;

	bool drawOpaque = true;
	bool drawTransparent = true;
	bool drawGrid = true;
};