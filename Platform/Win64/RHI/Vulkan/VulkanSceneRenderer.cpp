#include "VulkanSceneRenderer.hpp"
#include <RHI/RHICommon/RHIModuleWrapper.hpp>

VulkanSceneRenderer::VulkanSceneRenderer(RHI::IDynamicRHI* dynamicRHI)
	: RendererInterface(dynamicRHI)
{
	
}

VulkanSceneRenderer::~VulkanSceneRenderer()
{

}

bool VulkanSceneRenderer::initializeRender()
{
	if(m_Device)
	{
		if(!m_VertexShader || !m_PixelShader)
		{
			return false;
		}

		RHI::CommandListParameters commandListParams = { RHI::CommandQueue::Graphics };
		m_CommandList = m_Device->createCommandList(commandListParams);
		m_CommandLists.push_back(m_CommandList);

		return true;
	}

	return false;
}

void VulkanSceneRenderer::updateBuffers()
{
	
}

void VulkanSceneRenderer::composeFrame()
{
	
}

bool VulkanSceneRenderer::renderScene()
{
	if(m_GraphicsPipeline)
	{
		RHI::GraphicsPipelineDesc pipelineDesc;
		pipelineDesc.VS = m_VertexShader;
		pipelineDesc.PS = m_PixelShader;
		pipelineDesc.primType = RHI::PrimitiveType::TriangleList;
		pipelineDesc.pipelineInfo.useDepth = true;

		m_GraphicsPipeline = m_Device->createGraphicsPipeline(pipelineDesc, );
	}

    updateBuffers(imageIndex);

	m_CommandList->beginSingleTimeCommands();

    composeFrame(imageIndex);

	RHI::DrawArguments drawArgs = {};
	drawArgs.vertexCount = 3;
	m_CommandList->draw(drawArgs);
    m_CommandList->endSingleTimeCommands();

    m_Device->executeCommandLists(m_CommandLists, m_CommandLists.size(), RHI::CommandQueue::Graphics);
}
