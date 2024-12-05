#include <RHI/RHIVulkan/VulkanBackend.hpp>

namespace RHI::Vulkan
{
	Queue::Queue(const VulkanContext& context, CommandQueue queueID, VkQueue queue, uint32_t queueFamilyIndex)
		: m_Context(context)
		, m_Queue(queue)
		, m_QueueID(queueID)
		, m_QueueFamilyIndex(queueFamilyIndex)
	{
		
	}

	Queue::~Queue()
	{
		
	}

	void Queue::addWaitSemaphore(VkSemaphore semaphore, uint64_t value)
	{
		if (!semaphore)
			return;

		m_WaitSemaphores.push_back(semaphore);
		m_WaitSemaphoreValues.push_back(value);
	}

	void Queue::addSignalSemaphore(VkSemaphore semaphore, uint64_t value)
	{
		if (!semaphore)
			return;

		m_SignalSemaphores.push_back(semaphore);
		m_SignalSemaphoreValues.push_back(value);
	}

	VkSemaphore Device::getQueueSemaphore(CommandQueue queueID)
	{
		Queue& queue = *m_Queues[uint32_t(queueID)];

		return queue.
	}

}