#pragma once
#include <Renderer/RendererInterface.hpp>

#include "VulkanResources.hpp"

class VulkanSceneRenderer : public mythSystem::RendererInterface
{
public:
	VulkanSceneRenderer();
	virtual ~VulkanSceneRenderer();

	virtual void renderScene() override;
	virtual void initializeRender() override;

private:
};
