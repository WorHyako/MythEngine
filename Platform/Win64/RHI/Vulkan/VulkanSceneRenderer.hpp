#pragma once

#include <Renderer/RendererInterface.hpp>

constexpr uint32_t c_NumViews = 4;

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
	RHI::IBuffer* m_ConstantBuffer = nullptr;
	RHI::IBuffer* m_VertexBuffer = nullptr;
	RHI::IBuffer* m_IndexBuffer = nullptr;
	RHI::ITexture* m_Texture = nullptr;
	RHI::ISampler* m_Sampler = nullptr;
	RHI::IInputLayout* m_InputLayout = nullptr;
	RHI::IBindingLayout* m_BindingLayout = nullptr;
	RHI::IBindingSet* m_BindingSets[c_NumViews];
	RHI::IGraphicsPipeline* m_GraphicsPipeline = nullptr;
	RHI::IRHICommandList* m_CommandList = nullptr;
};
