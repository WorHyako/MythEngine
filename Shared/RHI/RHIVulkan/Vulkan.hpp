#pragma once

#include <RHICommon.hpp>

#define VK_NO_PROTOTYPES
#include "volk.h"

namespace RHI::Vulkan
{
	class IDevice : public RHI::IDevice
	{
	public:
		// Additional Vulkan-specific public methods
		virtual VkSemaphore getQueueSemaphore(CommandQueue queue) = 0;
		virtual void queueWaitForSemaphore(CommandQueue waitQueue, VkSemaphore semaphore, uint64_t value) = 0;
		virtual void queueSignalSemaphore(CommandQueue executionQueue, VkSemaphore semaphore, uint64_t value) = 0;
	};
}
