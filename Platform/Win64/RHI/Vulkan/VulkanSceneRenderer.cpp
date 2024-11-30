#include "VulkanSceneRenderer.hpp"
#include <RHI/RHICommon/RHIModuleWrapper.hpp>

VulkanSceneRenderer::VulkanSceneRenderer()
{
	
}

VulkanSceneRenderer::~VulkanSceneRenderer()
{
	rhiModule_ = nullptr;
	dynamicRHI_ = nullptr;
}

void VulkanSceneRenderer::InitializeRender()
{
	if(device_)
	{
		
	}
}

void VulkanSceneRenderer::RenderScene()
{
	
}
