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

void VulkanSceneRenderer::renderScene()
{
	m_CommandList->draw()
}
