#pragma once

#include <RHI/Vulkan/Framework/Renderer.hpp>

struct AtomicRenderer : public Renderer
{
	AtomicRenderer(VulkanRenderContext& ctx, VulkanBuffer sizeBuffer);

	void fillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE) override;

	void updateBuffers(size_t currentImage) override;

	std::vector<VulkanBuffer>& getOutputs() { return output_; }

private:
	std::vector<VulkanBuffer> atomics_;
	std::vector<VulkanBuffer> output_;
};

struct AnimRenderer : public Renderer
{
	AnimRenderer(VulkanRenderContext& ctx, std::vector<VulkanBuffer>& pointBuffers, VulkanBuffer sizeBuffer);

	void fillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE) override;

	float percentage = 0.5f;

private:
	std::vector<VulkanBuffer>& pointBuffers_;
};