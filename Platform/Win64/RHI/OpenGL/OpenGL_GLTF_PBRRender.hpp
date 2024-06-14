#pragma once

#include <RHI/OpenGL/OpenGLBaseRender.hpp>

class CameraPositioner_FirstPerson;
class TestCamera;

class OpenGL_GLTF_PBRRender : public OpenGLBaseRender
{
public:
	OpenGL_GLTF_PBRRender(GLApp* app);
	~OpenGL_GLTF_PBRRender() override = default;

	virtual int draw() override;

private:
	CameraPositioner_FirstPerson* positioner_;
	TestCamera* testCamera_;
};