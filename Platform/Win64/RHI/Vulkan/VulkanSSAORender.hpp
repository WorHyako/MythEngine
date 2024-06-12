#pragma once

#include <RHI/Vulkan/Framework/VulkanApp.hpp>

#include <RHI/Vulkan/Framework/MultiRenderer.hpp>
#include <RHI/Vulkan/Framework/GuiRenderer.hpp>
#include <RHI/Vulkan/Framework/QuadRenderer.hpp>
#include <RHI/Vulkan/Framework/Effects/SSAOProcessor.hpp>

struct SSAOApp : public CameraApp
{
	SSAOApp();

	virtual void drawUI() override;
	virtual void draw3D() override;

	bool enableSSAO;

private:
	VulkanTexture colorTex, depthTex, finalTex;

	VKSceneData sceneData;

	MultiRenderer multiRenderer;

	SSAOProcessor SSAO;

	QuadRenderer quads;

	GuiRenderer imgui;
};
