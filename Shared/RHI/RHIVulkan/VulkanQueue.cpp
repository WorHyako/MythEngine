#include <RHI/RHIVulkan/VulkanBackend.hpp>

namespace RHI::Vulkan
{
	Queue::Queue(const VulkanContext& context, CommandQueue queueID, VkQueue queue, uint32_t queueFamilyIndex)
		: m_Context(context)
		, m_Queue(queue)
		, m_QueueID(queueID)
		, m_QueueFamilyIndex(queueFamilyIndex)
	{
		const VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		vkCreateSemaphore(m_Context.device, &semaphoreCreateInfo, nullptr, &trackingSemaphore);
	}

	Queue::~Queue()
	{
		vkDestroySemaphore(m_Context.device, trackingSemaphore, nullptr);
		trackingSemaphore = VkSemaphore();
	}

	TrackedCommandBufferPtr Queue::createCommandBuffer()
	{
		TrackedCommandBufferPtr commandBuffer = std::make_shared<TrackedCommandBuffer>(m_Context);

		VkCommandPoolCreateInfo cpi{};
		cpi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cpi.flags = 0;
		cpi.queueFamilyIndex = m_QueueFamilyIndex;

		VK_CHECK(vkCreateCommandPool(m_Context.device, &cpi, nullptr, &commandBuffer->commandPool));

		VkCommandBufferAllocateInfo ai{};
		ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		ai.pNext = nullptr;
		ai.commandPool = commandBuffer->commandPool;
		ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		ai.commandBufferCount = static_cast<uint32_t>(1);

		VK_CHECK(vkAllocateCommandBuffers(m_Context.device, &ai, &commandBuffer->commandBuffer));

		return commandBuffer;
	}

	TrackedCommandBufferPtr Queue::getOrCreateCommandBuffer()
	{
		std::lock_guard lock_guard(m_Mutex); // this is called from CommandList::open, so free-threaded

		TrackedCommandBufferPtr commandBuffer;
		if(m_CommandBuffersPool.empty())
		{
			commandBuffer = createCommandBuffer();
		}
		else
		{
			commandBuffer = m_CommandBuffersPool.front();
			m_CommandBuffersPool.pop_front();
		}

		return commandBuffer;
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

		return queue.trackingSemaphore;
	}

	void Device::queueWaitForSemaphore(CommandQueue waitQueueID, VkSemaphore semaphore, uint64_t value)
	{
		Queue& queue = *m_Queues[uint32_t(waitQueueID)];

		queue.addWaitSemaphore(semaphore, value);
	}

	void Device::queueSignalSemaphore(CommandQueue executionQueueID, VkSemaphore semaphore, uint64_t value)
	{
		Queue& queue = *m_Queues[uint32_t(executionQueueID)];

		queue.addSignalSemaphore(semaphore, value);
	}


}