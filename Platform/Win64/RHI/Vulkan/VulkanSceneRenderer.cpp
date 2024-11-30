#include "VulkanSceneRenderer.hpp"
#include <RHI/RHICommon/RHIModuleWrapper.hpp>

VulkanSceneRenderer::VulkanSceneRenderer()
{
	
}

VulkanSceneRenderer::~VulkanSceneRenderer()
{

}

void VulkanSceneRenderer::initializeRender()
{
	if(m_Device)
	{
		
	}
}

void VulkanSceneRenderer::renderScene()
{
    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(m_Device, vkDev.swapchain, 0, vkDev.semaphore, VK_NULL_HANDLE, &imageIndex);
    VK_CHECK(vkResetCommandPool(m_Device, vkDev.commandPool, 0));

    if (result != VK_SUCCESS) return false;

    updateBuffersFunc(imageIndex);

    VkCommandBuffer commandBuffer = vkDev.commandBuffers[imageIndex];

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.pNext = nullptr;
    bi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    bi.pInheritanceInfo = nullptr;

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &bi));

    composeFrameFunc(commandBuffer, imageIndex);

    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // or even VERTEX_SHADER_STAGE

    VkSubmitInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.pNext = nullptr;
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &vkDev.semaphore;
    si.pWaitDstStageMask = waitStages;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &vkDev.commandBuffers[imageIndex];
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = &vkDev.renderSemaphore;

    VK_CHECK(vkQueueSubmit(vkDev.graphicsQueue, 1, &si, nullptr));

    VkPresentInfoKHR pi{};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.pNext = nullptr;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &vkDev.renderSemaphore;
    pi.swapchainCount = 1;
    pi.pSwapchains = &vkDev.swapchain;
    pi.pImageIndices = &imageIndex;

    VK_CHECK(vkQueuePresentKHR(vkDev.graphicsQueue, &pi));
    VK_CHECK(vkDeviceWaitIdle(vkDev.device));

    return true;
}
