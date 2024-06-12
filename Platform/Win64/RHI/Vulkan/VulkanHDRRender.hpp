#pragma once

#include <RHI/Vulkan/Framework/VulkanApp.hpp>

#include <RHI/Vulkan/Framework/GuiRenderer.hpp>
#include <RHI/Vulkan/Framework/MultiRenderer.hpp>
#include <RHI/Vulkan/Framework/QuadRenderer.hpp>
#include <RHI/Vulkan/Framework/CubeRenderer.hpp>
#include <RHI/Vulkan/Framework/Effects/HDRProcessor.hpp>

struct HDRApp : public CameraApp
{
	HDRApp();

	virtual void drawUI() override;
	virtual void draw3D() override;

	bool showPyramid = true;
	bool showDebug = true;
	bool enableToneMapping = true;

private:
	HDRUniformBuffer* hdrUniforms;

	VulkanTexture cubemap;
	VulkanTexture cubemapIrr;

	VulkanTexture HDRDepth;
	VulkanTexture HDRLuminance;
	VulkanTexture luminanceResult;
	VulkanTexture hdrTex;

	VKSceneData sceneData;
	CubemapRenderer cubeRenderer;
	MultiRenderer multiRenderer;

	LuminanceCalculator luminance;
	HDRProcessor hdr;

	std::vector<VulkanTexture> displayedTextureList;

	GuiRenderer imgui;
	QuadRenderer quads;

	ShaderOptimalToDepthBarrier toDepth;
	DepthToShaderOptimalBarrier toShader;

	ShaderOptimalToColorBarrier lumToColor;
	ColorToShaderOptimalBarrier lumToShader;

	ColorWaitBarrier lumWait;
};
