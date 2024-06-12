#pragma once

#include <RHI/Vulkan/Framework/VulkanApp.hpp>

#include <RHI/Vulkan/Framework/GuiRenderer.hpp>
#include <RHI/Vulkan/Framework/MultiRenderer.hpp>
#include <RHI/Vulkan/Framework/InfinitePlaneRenderer.hpp>

#include <Physics/Physics.hpp>

struct PhysicsApp : public CameraApp
{
	PhysicsApp();

	virtual void draw3D() override;
	virtual void update(float deltaSeconds) override;

private:
	InfinitePlaneRenderer plane;

	VKSceneData sceneData;
	MultiRenderer multiRenderer;

	GuiRenderer imgui;

	Physics physics;
};

void generateMeshFile();
void generateData();
