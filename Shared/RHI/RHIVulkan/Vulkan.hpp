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
		virtual VkSemaphore getQueueSemaphore(CommandQueue queueID) = 0;
		virtual void queueWaitForSemaphore(CommandQueue waitQueueID, VkSemaphore semaphore, uint64_t value) = 0;
		virtual void queueSignalSemaphore(CommandQueue executionQueueID, VkSemaphore semaphore, uint64_t value) = 0;
	};

	VkFormat convertFormat(RHI::Format format);
}
