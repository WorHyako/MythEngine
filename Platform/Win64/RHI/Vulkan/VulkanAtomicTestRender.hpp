#pragma once

#include <RHI/Vulkan/Framework/VulkanApp.hpp>
#include <RHI/Vulkan/Framework/AtomicRenderer.hpp>
#include <RHI/Vulkan/Framework/GuiRenderer.hpp>

struct AtomicTestApp : public CameraApp
{
	AtomicTestApp();

	void draw3D(){}

	void drawUI() override;

private:
	VulkanBuffer sizeBuffer;

	AtomicRenderer atom;
	AnimRenderer anim;
	GuiRenderer imgui;
};