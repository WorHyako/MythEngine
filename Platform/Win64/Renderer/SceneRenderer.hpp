#pragma once

#include <Renderer/RendererInterface.hpp>
#include <Renderer/Sea.hpp>
#include <memory>

#include "EngineTypes.hpp"

constexpr uint32_t c_NumViews = 4;

class SceneRenderer : public mythSystem::RendererInterface
{
public:
	SceneRenderer(RHI::IDynamicRHI* dynamicRHI);
	virtual ~SceneRenderer();

	virtual bool renderScene() override;
	virtual bool initializeRender() override;
	virtual void updateBuffers() override;
	virtual void composeFrame() override;

private:
	RHI::ShaderHandle m_VertexShader = nullptr;
	RHI::ShaderHandle m_PixelShader = nullptr;
	RHI::BufferHandle m_ConstantBuffer = nullptr;
	RHI::BufferHandle m_VertexBuffer = nullptr;
	RHI::BufferHandle m_IndexBuffer = nullptr;
	RHI::TextureHandle m_Texture = nullptr;
	RHI::SamplerHandle m_Sampler = nullptr;
	RHI::InputLayoutHandle m_InputLayout = nullptr;
	RHI::BindingLayoutHandle m_BindingLayout = nullptr;
	RHI::BindingSetHandle m_BindingSets[c_NumViews];
	RHI::GraphicsPipelineHandle m_GraphicsPipeline = nullptr;
	RHI::CommandListHandle m_CommandList = nullptr;

	mythSystem::UniquePtr<Sea> m_SeaPass = nullptr;
};
