#include <RHI/RHIVulkan/VulkanBackend.hpp>

namespace RHI::Vulkan
{
    Device::Device(const DeviceDesc& desc)
        : m_Context(desc.instance, desc.physicalDevice, desc.device, *desc.ctxExtensions, *desc.ctxFeatures)
		, m_DeviceDesc(desc)
		, m_Resources(this)
    {
        if (desc.useGraphicsQueue)
        {
            m_Queues[uint32_t(CommandQueue::Graphics)] = std::make_unique<Queue>(m_Context,
                CommandQueue::Graphics, m_DeviceDesc.graphicsQueue, m_DeviceDesc.graphicsFamily);

            if (m_Queues[uint32_t(CommandQueue::Graphics)].get() == nullptr)
            {
                exit(EXIT_FAILURE);
            }
        }

        if (desc.useComputeQueue)
        {
            m_Queues[uint32_t(CommandQueue::Compute)] = std::make_unique<Queue>(m_Context,
                CommandQueue::Compute, m_DeviceDesc.computeQueue, m_DeviceDesc.computeFamily);

            if (m_Queues[uint32_t(CommandQueue::Compute)].get() == nullptr)
            {
                exit(EXIT_FAILURE);
            }
        }

        if(desc.useTransferQueue)
        {
            m_Queues[uint32_t(CommandQueue::Copy)] = std::make_unique<Queue>(m_Context,
                CommandQueue::Copy, m_DeviceDesc.transferQueue, m_DeviceDesc.transferFamily);

            if(m_Queues[uint32_t(CommandQueue::Copy)].get() == nullptr)
            {
                exit(EXIT_FAILURE);
            }
        }

        m_Context.ctxExtensions = *desc.ctxExtensions;
        m_Context.ctxFeatures = *desc.ctxFeatures;

        VkCommandPoolCreateInfo cpi{};
        cpi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cpi.flags = 0;
        cpi.queueFamilyIndex = m_DeviceDesc.graphicsFamily;

        VK_CHECK(vkCreateCommandPool(m_Context.device, &cpi, nullptr, &m_Resources.commandPool));

        VkCommandBufferAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.pNext = nullptr;
        ai.commandPool = m_Resources.commandPool;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = static_cast<uint32_t>(m_Resources.swapchainImages.size());

        VK_CHECK(vkAllocateCommandBuffers(m_Context.device, &ai, &m_Resources.commandBuffers[0]));

        if (desc.useComputeQueue)
        {
            // Create compute command pool
            VkCommandPoolCreateInfo cpi1;
            cpi1.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cpi1.pNext = nullptr;
            cpi1.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; /* Allow command from this pool buffers to be reset*/
            cpi1.queueFamilyIndex = static_cast<uint32_t>(m_Queues[uint32_t(CommandQueue::Compute)]->getQueueFamilyIndex());

            VK_CHECK(vkCreateCommandPool(m_Context.device, &cpi1, nullptr, &m_Resources.computeCommandPool));

            VkCommandBufferAllocateInfo ai1{};
            ai1.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            ai1.pNext = nullptr;
            ai1.commandPool = m_Resources.computeCommandPool;
            ai1.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            ai1.commandBufferCount = 1;

            VK_CHECK(vkAllocateCommandBuffers(m_Context.device, &ai1, &m_Resources.computeCommandBuffer));
        }

        return true;
    }

    GraphicsAPI Device::getGraphicsAPI() const
    {
        return RHI::GraphicsAPI::VULKAN;
    }

    Device::~Device()
    {
        
    }

    IRHICommandList* Device::createCommandList(const CommandListParameters& params)
    {
        if (!m_Queues[uint32_t(params.queueType)])
            return nullptr;

        CommandList* cmdList = new CommandList(this, m_Context, params);

        return cmdList;
    }
}
