#pragma once
#include <Renderer/RendererInterface.hpp>

#include "VulkanResources.hpp"

class VulkanSceneRenderer : public mythSystem::RendererInterface
{
public:
	VulkanSceneRenderer(RHI::IDevice* device);
	virtual ~VulkanSceneRenderer();

	virtual void renderScene() override;
	virtual void initializeRender() override;

private:


};
