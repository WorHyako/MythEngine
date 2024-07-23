#include "VulkanSceneRenderer.hpp"
#include <RHI/RHICommon/RHIModuleWrapper.hpp>

VulkanSceneRenderer::VulkanSceneRenderer()
{
	rhiModule_ = RHI::InitializeModuleRHI(RHI::GraphicsAPI::VULKAN);
	if(rhiModule_)
	{
		dynamicRHI_ = rhiModule_->createRHI();
		if(dynamicRHI_)
		{
			device_ = dynamicRHI_->getDevice();
		}

	}
}

VulkanSceneRenderer::~VulkanSceneRenderer()
{
	rhiModule_ = nullptr;
	dynamicRHI_ = nullptr;
}

void VulkanSceneRenderer::initializeResources()
{
	if(device_)
	{
		
	}
}

void VulkanSceneRenderer::draw()
{
	
}
