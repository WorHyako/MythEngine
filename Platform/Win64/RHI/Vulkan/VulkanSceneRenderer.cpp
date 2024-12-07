#include "VulkanSceneRenderer.hpp"
#include <RHI/RHICommon/RHIModuleWrapper.hpp>

VulkanSceneRenderer::VulkanSceneRenderer(RHI::IDynamicRHI* dynamicRHI)
	: RendererInterface(dynamicRHI)
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
}
