#include "VulkanSceneRenderer.hpp"
#include <RHI/RHICommon/RHIModuleWrapper.hpp>

VulkanSceneRenderer::VulkanSceneRenderer(RHI::IDevice* device)
	: RendererInterface(device)
{
	
}

VulkanSceneRenderer::~VulkanSceneRenderer()
{

}

void VulkanSceneRenderer::initializeRender()
{
	if(m_Device)
	{
		RHI::CommandListParameters commandListParams = { RHI::CommandQueue::Graphics };
		m_CommandList = m_Device->createCommandList(commandListParams);
	}
}

void VulkanSceneRenderer::updateBuffers()
{
	
}

void VulkanSceneRenderer::composeFrame()
{
	
}

bool VulkanSceneRenderer::renderScene()
{
    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(m_Context.device, m_Device->getResources()->swapchain, 0, m_Device->getQueue(RHI::CommandQueue::Graphics)->semaphore, VK_NULL_HANDLE, &imageIndex);
    VK_CHECK(vkResetCommandPool(m_Context.device, m_Device->getResources()->commandPool, 0));

    if (result != VK_SUCCESS) return false;

    updateBuffers(imageIndex);

    VkCommandBuffer commandBuffer = m_Device->getResources()->commandBuffers[imageIndex];

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.pNext = nullptr;
    bi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    bi.pInheritanceInfo = nullptr;

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &bi));

    composeFrame(imageIndex);

    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // or even VERTEX_SHADER_STAGE

    VkSubmitInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.pNext = nullptr;
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &m_Device->getQueue(RHI::CommandQueue::Graphics)->semaphore;
    si.pWaitDstStageMask = waitStages;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &m_Device->getResources()->commandBuffers[imageIndex];
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = &m_Device->getQueue(RHI::CommandQueue::Graphics)->renderSemaphore;

    VK_CHECK(vkQueueSubmit(m_Device->getQueue(RHI::CommandQueue::Graphics)->getVkQueue(), 1, &si, nullptr));

    VkPresentInfoKHR pi{};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.pNext = nullptr;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &m_Device->getQueue(RHI::CommandQueue::Graphics)->renderSemaphore;
    pi.swapchainCount = 1;
    pi.pSwapchains = &m_Device->getResources()->swapchain;
    pi.pImageIndices = &imageIndex;

    VK_CHECK(vkQueuePresentKHR(m_Device->getQueue(RHI::CommandQueue::Graphics)->getVkQueue(), &pi));
    VK_CHECK(vkDeviceWaitIdle(m_Context.device));

    return true;
}
