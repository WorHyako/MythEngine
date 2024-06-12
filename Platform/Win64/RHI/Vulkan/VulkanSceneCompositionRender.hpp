#pragma once

#include <RHI/Vulkan/Framework/VulkanApp.hpp>
#include <RHI/Vulkan/Framework/FinalRenderer.hpp>
#include <RHI/Vulkan/Framework/GuiRenderer.hpp>
#include <RHI/Vulkan/Framework/QuadRenderer.hpp>
#include <RHI/Vulkan/Framework/CubeRenderer.hpp>
#include <RHI/Vulkan/Framework/LineCanvas.hpp>

#include <RHI/Vulkan/Framework/Effects/SSAOProcessor.hpp>
#include <RHI/Vulkan/Framework/Effects/HDRProcessor.hpp>

struct SceneCompositionApp : public CameraApp
{
	SceneCompositionApp();

	void drawUI() override;

	void draw3D() override;

	bool showPyramid = false;
	bool showHDRDebug = false;

	bool enableHDR = true;

	bool enableSSAO = true;
	bool showSSAODebug = false;
	bool showShadowBuffer = false;

	bool showLightFrustum = false;
	bool showObjectBoxes = false;

	float lightPhi = -15.0f;
	float lightTheta = +30.0f;

private:
	HDRUniformBuffer* hdrUniforms;

	VulkanTexture colorTex, depthTex, finalTex;

	VulkanTexture luminanceResult;

	VulkanTexture envMap;
	VulkanTexture irrMap;

	VKSceneData sceneData;

	CubemapRenderer cubeRenderer;
	FinalMultiRenderer finalRenderer;

	LuminanceCalculator luminance;

	BufferAttachment hdrUniformBuffer;
	HDRProcessor hdr;

	SSAOProcessor ssao;

	std::vector<VulkanTexture> displayedTextureList;

	QuadRenderer quads;

	GuiRenderer imgui;
	LineCanvas canvas;

	BoundingBox bigBox;

	ShaderOptimalToDepthBarrier toDepth;
	ShaderOptimalToColorBarrier lumToColor;

	ColorWaitBarrier lumWait;
};