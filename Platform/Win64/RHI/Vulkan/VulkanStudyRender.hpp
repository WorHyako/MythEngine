#pragma once
#include <RHI/Vulkan/VulkanBaseRender.hpp>

class RendererBase;

class VulkanStudyRender : public VulkanBaseRender
{
public:
	VulkanStudyRender(const uint32_t screenWidth, uint32_t screenHeight);

	virtual bool initVulkan() override;
	virtual void terminateVulkan() override;
	virtual int draw() override;

public:
	bool drawFrame(const std::vector<RendererBase*>& renderers);
	void composeFrame(uint32_t imageIndex, const std::vector<RendererBase*>& renderers);
	void renderGUI(uint32_t imageIndex);
	void update2D(uint32_t imageIndex);
	void update3D(uint32_t imageIndex);
};