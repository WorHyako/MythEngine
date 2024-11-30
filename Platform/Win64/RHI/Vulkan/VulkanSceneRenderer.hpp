#pragma once
#include <Renderer/RendererInterface.hpp>

#include "VulkanResources.hpp"

class VulkanSceneRenderer : public mythSystem::RendererInterface
{
public:
	VulkanSceneRenderer();
	virtual ~VulkanSceneRenderer();

	virtual void RenderScene() override;
	virtual void InitializeRender() override;

private:
	RHI::IRHIModule* rhiModule_;
	RHI::IDynamicRHI* dynamicRHI_;
};
