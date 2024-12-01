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

}