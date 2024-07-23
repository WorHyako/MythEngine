#pragma once
#include "VulkanResources.hpp"
#include <RHI/RHICommon/RHICommon.hpp>

class VulkanSceneRenderer
{
public:
	VulkanSceneRenderer();
	~VulkanSceneRenderer();

	void initializeResources();
	void draw();

private:
	RHI::IRHIModule* rhiModule_;
	RHI::IDynamicRHI* dynamicRHI_;
	RHI::IDevice* device_;
};
