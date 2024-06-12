#pragma once

#include <RHI/Vulkan/Framework/VulkanApp.hpp>

#include <RHI/Vulkan/Framework/GuiRenderer.hpp>
#include <RHI/Vulkan/Framework/MultiRenderer.hpp>
#include <RHI/Vulkan/Framework/InfinitePlaneRenderer.hpp>

struct SceneGraphApp : public CameraApp
{
	SceneGraphApp();

	void drawUI() override;

	void draw3D() override;

	void update(float deltaSeconds) override;

private:
	VulkanTexture envMap;
	VulkanTexture irrMap;

	VKSceneData sceneData;

	InfinitePlaneRenderer plane;
	MultiRenderer muiltiRenderer;
	GuiRenderer imgui;

	int selectedNode = -1;

	void editNode(int node);
	void editTransform(const glm::mat4& view, const glm::mat4& projection, glm::mat4& matrix);
	void editMaterial(int node);
};
