#pragma once
#include <Renderer/RendererInterface.hpp>

#include "VulkanResources.hpp"

class VulkanSceneRenderer : public mythSystem::RendererInterface
{
public:
	VulkanSceneRenderer(RHI::IDynamicRHI* dynamicRHI);
	virtual ~VulkanSceneRenderer();

	virtual bool renderScene() override;
	virtual bool initializeRender() override;
	virtual void updateBuffers() override;
	virtual void composeFrame() override;

private:
	RHI::IShader* m_VertexShader = nullptr;
	RHI::IShader* m_PixelShader = nullptr;
	RHI::IGraphicsPipeline* m_GraphicsPipeline = nullptr;
	RHI::IRHICommandList* m_CommandList = nullptr;
};
