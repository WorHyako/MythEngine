#pragma once

#include <RHI/Vulkan/Framework/Renderer.hpp>

/**
VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkImageLayout oldLayout VkImageLayout newLayout

// Before next stage (convert from )
ImageBarrier(ctx_, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
// Return back to attachment
ImageBarrier(ctx_, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL)
*/

struct ShaderOptimalToColorBarrier : public Renderer
{
	ShaderOptimalToColorBarrier(VulkanRenderContext& c, VulkanTexture tex) :
		Renderer(c),
		tex_(tex)
	{}

	void fillCommandBuffer(VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE) override
	{
		transitionImageLayoutCmd(cmdBuffer, tex_.image.image, tex_.format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}

private:
	VulkanTexture tex_;
};

struct ShaderOptimalToDepthBarrier : public Renderer
{
	ShaderOptimalToDepthBarrier(VulkanRenderContext& c, VulkanTexture tex) :
		Renderer(c),
		tex_(tex)
	{}

	void fillCommandBuffer(VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE) override
	{
		transitionImageLayoutCmd(cmdBuffer, tex_.image.image, tex_.format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}

private:
	VulkanTexture tex_;
};

struct ColorToShaderOptimalBarrier : public Renderer
{
	ColorToShaderOptimalBarrier(VulkanRenderContext& c, VulkanTexture tex) :
		Renderer(c),
		tex_(tex)
	{}

	void fillCommandBuffer(VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE) override
	{
		transitionImageLayoutCmd(cmdBuffer, tex_.image.image, tex_.format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

private:
	VulkanTexture tex_;
};

struct ColorWaitBarrier : public Renderer
{
	ColorWaitBarrier(VulkanRenderContext& c, VulkanTexture tex) :
		Renderer(c),
		tex_(tex)
	{}

	void fillCommandBuffer(VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE) override
	{
		transitionImageLayoutCmd(cmdBuffer, tex_.image.image, tex_.format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

private:
	VulkanTexture tex_;
};

struct DepthToShaderOptimalBarrier : public Renderer
{
	DepthToShaderOptimalBarrier(VulkanRenderContext& c, VulkanTexture tex) :
		Renderer(c),
		tex_(tex)
	{}

	void fillCommandBuffer(VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE) override
	{
		transitionImageLayoutCmd(cmdBuffer, tex_.image.image, tex_.format, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

private:
	VulkanTexture tex_;
};

