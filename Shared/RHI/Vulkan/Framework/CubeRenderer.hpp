#pragma once

#include <RHI/Vulkan/Framework/Renderer.hpp>

struct CubemapRenderer : public Renderer
{
	CubemapRenderer(VulkanRenderContext& ctx,
		VulkanTexture texture,
		const std::vector<VulkanTexture>& outputs = std::vector<VulkanTexture>{},
		RenderPass screenRenderPass = RenderPass());

	void fillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE) override;

	void updateBuffers(size_t currentImage) override;

	inline void setMatrices(const glm::mat4& proj, const glm::mat4& view) { ubo.proj_ = proj; ubo.view_ = view; }

private:
	struct UniformBuffer
	{
		glm::mat4 proj_;
		glm::mat4 view_;
	} ubo;
};