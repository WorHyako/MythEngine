#include <RHI/Vulkan/Renderers/VulkanRendererBase.hpp>
#include <stdio.h>

RendererBase::~RendererBase()
{
	for (auto buf : uniformBuffers_)
		vkDestroyBuffer(device_, buf, nullptr);

	for (auto mem : uniformBuffersMemory_)
		vkFreeMemory(device_, mem, nullptr);

	vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
	vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);

	for (auto framebuffer : swapchainFramebuffers_)
		vkDestroyFramebuffer(device_, framebuffer, nullptr);

	vkDestroyRenderPass(device_, renderPass_, nullptr);
	vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
	vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
}

void RendererBase::beginRenderPass(VkCommandBuffer commandBuffer, size_t currentImage)
{
	VkRect2D screenRect{};
	screenRect.offset = { 0, 0 };
	screenRect.extent = { framebufferWidth_, framebufferHeight_ };

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.pNext = nullptr;
	renderPassInfo.renderPass = renderPass_;
	renderPassInfo.framebuffer = swapchainFramebuffers_[currentImage];
	renderPassInfo.renderArea = screenRect;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_, 0, 1, &descriptorSets_[currentImage], 0, nullptr);
}

bool RendererBase::createUniformBuffers(VulkanRenderDevice& vkDev, size_t uniformDataSize)
{
	uniformBuffers_.resize(vkDev.swapchainImages.size());
	uniformBuffersMemory_.resize(vkDev.swapchainImages.size());
	for(size_t i = 0; i < vkDev.swapchainImages.size(); i++)
	{
		if(!createUniformBuffer(vkDev, uniformBuffers_[i], uniformBuffersMemory_[i], uniformDataSize))
		{
			printf("Cannot create uniform buffer\n");
			fflush(stdout);
			return false;
		}
	}
	return true;
}

//void RendererBase::cleanupRendererResources()
//{
//	vkDestroyImageView(device_, vkDev.ima, nullptr);
//	vkDestroyImage(device_, colorImage, nullptr);
//	vkFreeMemory(device_, colorImageMemory, nullptr);
//
//	vkDestroyImageView(device_, depthTexture_.imageView, nullptr);
//	vkDestroyImage(device_, depthTexture_.image, nullptr);
//	vkFreeMemory(device_, depthTexture_.imageMemory, nullptr);
//
//	for (auto framebuffer : swapchainFramebuffers_)
//	{
//		vkDestroyFramebuffer(device_, framebuffer, nullptr);
//	}
//
//	vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
//	vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
//	vkDestroyRenderPass(device_, renderPass_ , nullptr);
//
//	for (auto imageView : vkDev.swapchainImageViews)
//	{
//		vkDestroyImageView(vkDev.device, imageView, nullptr);
//	}
//
//	vkDestroySwapchainKHR(device_, swapchain_, nullptr);
//}