#pragma once

#include <RHI/Vulkan/Framework/VulkanApp.hpp>

#include <RHI/Vulkan/Framework/GuiRenderer.hpp>
#include <RHI/Vulkan/Framework/MultiRenderer.hpp>

struct LargeSceneApp : public CameraApp
{
	LargeSceneApp();

	virtual void draw3D() override;

private:
	VulkanTexture envMap;
	VulkanTexture irrMap;

	VKSceneData sceneData;
	VKSceneData sceneData2;

	MultiRenderer muiltiRenderer;
	MultiRenderer multiRenderer2;
	GuiRenderer imgui;
};
