#pragma once

#include <RHI/Vulkan/Renderers/VulkanRendererBase.hpp>

class VulkanFinish : public RendererBase
{
public:
	VulkanFinish(VulkanRenderDevice& vkDev, VulkanImage depthTexture);

	virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;
};