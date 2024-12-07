#pragma once
#include <Renderer/RendererInterface.hpp>

#include "VulkanResources.hpp"

class VulkanSceneRenderer : public mythSystem::RendererInterface
{
public:
	VulkanSceneRenderer(RHI::IDynamicRHI* dynamicRHI);
	virtual ~VulkanSceneRenderer();

	virtual bool renderScene() override;
	virtual void initializeRender() override;
	virtual void updateBuffers() override;
	virtual void composeFrame() override;

private:

};
