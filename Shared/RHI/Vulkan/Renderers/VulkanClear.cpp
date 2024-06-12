#include <RHI/Vulkan/Renderers/VulkanClear.hpp>

#include <RHI/Vulkan/UtilsVulkan.hpp>

VulkanClear::VulkanClear(VulkanRenderDevice& vkDev, VulkanImage depthTexture)
	:RendererBase(vkDev, depthTexture),
	shouldClearDepth(depthTexture.image != VK_NULL_HANDLE)
{
	RenderPassCreateInfo ci{};
	ci.clearColor_ = true;
	ci.clearDepth_ = true;
	ci.flags_ = eRenderPassBit_First;
	if(!createColorAndDepthRenderPass(vkDev, shouldClearDepth, &renderPass_, ci))
	{
		printf("VulkanClear: failed to create render pass\n");
		exit(EXIT_FAILURE);
	}

	createColorAndDepthFramebuffers(vkDev, renderPass_, depthTexture.imageView, swapchainFramebuffers_);
}

void VulkanClear::fillCommandBuffer(VkCommandBuffer commandBuffer, size_t swapFramebuffer)
{
	VkClearValue clearValue1{};
	clearValue1.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	VkClearValue clearValue2{};
	clearValue2.depthStencil = { 1.0f, 0 };
	VkClearValue clearValues[2] = { clearValue1, clearValue2 };

	VkRect2D screenRect{};
	screenRect.offset = { 0, 0 };
	screenRect.extent = { framebufferWidth_, framebufferHeight_ };

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass_;
	renderPassInfo.framebuffer = swapchainFramebuffers_[swapFramebuffer];
	renderPassInfo.renderArea = screenRect;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(shouldClearDepth ? 2 : 1);
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(commandBuffer);
}