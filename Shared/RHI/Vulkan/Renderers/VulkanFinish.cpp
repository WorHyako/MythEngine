#include <RHI/Vulkan/Renderers/VulkanFinish.hpp>

VulkanFinish::VulkanFinish(VulkanRenderDevice& vkDev, VulkanImage depthTexture)
	:RendererBase(vkDev, depthTexture)
{
	RenderPassCreateInfo ci{};
	ci.clearColor_ = false;
	ci.clearDepth_ = false;
	ci.flags_ = eRenderPassBit_Last;

	if(!createColorAndDepthRenderPass(vkDev, (depthTexture.image != VK_NULL_HANDLE), &renderPass_, ci))
	{
		printf("VulkanFinish: failed to create render pass\n");
		exit(EXIT_FAILURE);
	}

	createColorAndDepthFramebuffers(vkDev, renderPass_, depthTexture.imageView, swapchainFramebuffers_);
}

void VulkanFinish::fillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage)
{
	VkRect2D screenRect{};
	screenRect.offset = { 0, 0 };
	screenRect.extent = { framebufferWidth_, framebufferHeight_ };

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass_;
	renderPassInfo.framebuffer = swapchainFramebuffers_[currentImage];
	renderPassInfo.renderArea = screenRect;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(commandBuffer);
}