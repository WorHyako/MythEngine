#pragma once

#include <RHI/Vulkan/Framework/VulkanApp.hpp>

#include <RHI/Vulkan/Framework/LineCanvas.hpp>
#include <RHI/Vulkan/Framework/GuiRenderer.hpp>
#include <RHI/Vulkan/Framework/QuadRenderer.hpp>
#include <RHI/Vulkan/Framework/VulkanShaderProcessor.hpp>

struct ShadowMappingApp : public CameraApp
{
	ShadowMappingApp();

	virtual void update(float deltaSeconds) override;
	virtual void drawUI() override;
	virtual void draw3D() override;

private:
	std::vector<float> meshVertices;
	std::vector<unsigned int> meshIndices;

	std::pair<BufferAttachment, BufferAttachment> meshBuffer;
	std::pair<BufferAttachment, BufferAttachment> planeBuffer;

	VulkanBuffer meshUniformBuffer;
	VulkanBuffer shadowUniformBuffer;

	VulkanTexture meshDepth, meshColor;
	VulkanTexture meshShadowDepth, meshShadowColor;

	OffscreenMeshRenderer meshRenderer;
	OffscreenMeshRenderer depthRenderer;
	OffscreenMeshRenderer planeRenderer;

	LineCanvas canvas;

	QuadRenderer quads;
	GuiRenderer imgui;
};
