#include <RHI/Vulkan/VulkanSceneRenderer.hpp>
#include <RHI/RHICommon/RHIModuleWrapper.hpp>

#include <Filesystem/FilesystemUtilities.hpp>

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
		m_VertexShader = m_Device->createShaderModule((FilesystemUtilities::GetShadersDir() + "Vulkan/VK01.vert").c_str());
		m_PixelShader = m_Device->createShaderModule((FilesystemUtilities::GetShadersDir() + "Vulkan/VK01.frag").c_str());

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
	if(!m_GraphicsPipeline)
	{
		RHI::GraphicsPipelineDesc pipelineDesc;
		pipelineDesc.VS = m_VertexShader;
		pipelineDesc.PS = m_PixelShader;
		pipelineDesc.primType = RHI::PrimitiveType::TriangleList;
		pipelineDesc.pipelineInfo.useDepth = true;

		m_GraphicsPipeline = m_Device->createGraphicsPipeline(pipelineDesc, m_DynamicRHI->GetFramebuffer(m_DynamicRHI->GetCurrentBackBufferIndex()));
	}

    //updateBuffers(imageIndex);

	m_CommandList->beginSingleTimeCommands();

    //composeFrame(imageIndex);

	RHI::GraphicsState state = {};
	state.pipeline = m_GraphicsPipeline;
	state.framebuffer = m_DynamicRHI->GetFramebuffer(m_DynamicRHI->GetCurrentBackBufferIndex());
	m_CommandList->setGraphicsState(state);

	RHI::DrawArguments drawArgs = {};
	drawArgs.vertexCount = 3;
	m_CommandList->draw(drawArgs);
    m_CommandList->endSingleTimeCommands();

    m_Device->executeCommandLists(m_CommandLists, m_CommandLists.size(), RHI::CommandQueue::Graphics);

	return true;
}
