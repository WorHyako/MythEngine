#pragma once

#include <RHI/OpenGL/OpenGLBaseRender.hpp>

#include <glm/glm.hpp>

class CameraPositioner_FirstPerson;
class TestCamera;

struct SSAOParams
{
	float scale_ = 1.5f;
	float bias_ = 0.15f;
	float zNear = 0.1f;
	float zFar = 1000.0f;
	float radius = 0.05f;
	float attScale = 1.01f;
	float distScale = 0.6f;
};

struct HDRParams
{
	float exposure_ = 0.9f;
	float maxWhite_ = 1.17f;
	float bloomStrength_ = 1.1f;
	float adaptationSpeed_ = 0.1f;
};

class OpenGLSceneCompositionRender : public OpenGLBaseRender
{
public:
	OpenGLSceneCompositionRender(GLApp* app);
	~OpenGLSceneCompositionRender() override = default;

	virtual int draw() override;

private:
	CameraPositioner_FirstPerson* positioner_;
	TestCamera* testCamera_;

	bool enableGPUCulling = true;
	bool freezeCullingView = false;
	bool drawOpaque = true;
	bool drawTransparent = true;
	bool drawGrid = true;
	bool enableSSAO = true;
	bool enableBlur = true;
	bool enableHDR = true;
	bool drawBoxes = false;
	bool enableShadows = true;
	bool showLightFrustum = false;
	float lightTheta = 0.0f;
	float lightPhi = 0.0f;

	glm::mat4 cullingView;
	SSAOParams SSAOParams;
	HDRParams HDRParams;
};