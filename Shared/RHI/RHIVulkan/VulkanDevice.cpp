#include <RHI/RHIVulkan/VulkanBackend.hpp>

namespace RHI::Vulkan
{
    Device::Device(DeviceDesc& desc)
        : m_Context(desc.instance, desc.physicalDevice, desc.device, *desc.ctxExtensions, *desc.ctxFeatures)
		, m_DeviceDesc(desc)
		, m_Resources(this)
    {
        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        VkPhysicalDeviceFeatures deviceFeatures = initVulkanRenderDeviceFeatures(m_Context.ctxFeatures, deviceFeatures2);

        initVulkanRenderDevice(desc, isDeviceSuitable, deviceFeatures, deviceFeatures2);
    }

    Device::~Device()
    {
        destroyVulkanRenderDevice();
    }

    VkPhysicalDeviceFeatures Device::initVulkanRenderDeviceFeatures(const VulkanContextFeatures& ctxFeatures, VkPhysicalDeviceFeatures2& deviceFeatures2)
    {
        VkPhysicalDeviceFeatures deviceFeatures{};
        /* for wireframe outlines */
        deviceFeatures.geometryShader = (VkBool32)(ctxFeatures.geometryShader_ ? VK_TRUE : VK_FALSE);
        /* for tesselation experiments */
        deviceFeatures.tessellationShader = (VkBool32)(ctxFeatures.tessellationShader_ ? VK_TRUE : VK_FALSE);
        /* for indirect instanced rendering */
        deviceFeatures.multiDrawIndirect = (VkBool32)(ctxFeatures.multiDrawIndirect ? VK_TRUE : VK_FALSE);
        deviceFeatures.drawIndirectFirstInstance = (VkBool32)(ctxFeatures.drawIndirectFirstInstance ? VK_TRUE : VK_FALSE);
        /* for OIT and general atomic operations */
        deviceFeatures.vertexPipelineStoresAndAtomics = (VkBool32)(ctxFeatures.vertexPipelineStoresAndAtomics_ ? VK_TRUE : VK_FALSE);
        deviceFeatures.fragmentStoresAndAtomics = (VkBool32)(ctxFeatures.fragmentStoresAndAtomics_ ? VK_TRUE : VK_FALSE);
        /* for arrays of textures */
        deviceFeatures.shaderSampledImageArrayDynamicIndexing = (VkBool32)(ctxFeatures.shaderSampledImageArrayDynamicIndexing ? VK_TRUE : VK_FALSE);
        /* for GL <-> VK material shader compatibility */
        deviceFeatures.shaderInt64 = (VkBool32)(ctxFeatures.shaderInt64 ? VK_TRUE : VK_FALSE);

        if(ctxFeatures.deviceDescriptorIndexing)
        {
            VkPhysicalDeviceDescriptorIndexingFeaturesEXT physicalDeviceDescriptorIndexingFeatures{};
            physicalDeviceDescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
            physicalDeviceDescriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
            physicalDeviceDescriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
            physicalDeviceDescriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;

            deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            deviceFeatures2.pNext = &physicalDeviceDescriptorIndexingFeatures;
            deviceFeatures2.features = deviceFeatures;
        }

        //return initVulkanRenderDeviceWithCompute(vk, vkDev, width, height, ctxExtensions, isDeviceSuitable, deviceFeatures, deviceFeatures2, ctxFeatures.supportsScreenshots_);
    }

    bool Device::initVulkanRenderDevice(
        DeviceDesc& desc,
        std::function<bool(VkPhysicalDevice)> selector,
        VkPhysicalDeviceFeatures deviceFeatures,
        VkPhysicalDeviceFeatures2 deviceFeatures2)
    {
        //m_Context.vkDev.framebufferWidth = width;
        //m_Context.vkDev.framebufferHeight = height;

        VK_CHECK(findSuitablePhysicalDevice(m_Context.vulkanInstance.instance, selector, &m_Context.physicalDevice));
        if(desc.useGraphicsQueue)
        {
	        m_DeviceDesc.graphicsFamily = findQueueFamilies(m_Context.physicalDevice, VK_QUEUE_GRAPHICS_BIT);
            m_DeviceQueueIndices.push_back(m_DeviceDesc.graphicsFamily);
        }
        //	VK_CHECK(createDevice2(m_Context.m_PhysicalDevice, deviceFeatures2, vkDev.graphicsFamily, &m_Context.m_Device));
        //	VK_CHECK(vkGetBestComputeQueue(m_Context.m_PhysicalDevice, &vkDev.computeFamily));
        if(desc.useComputeQueue)
        {
            m_DeviceDesc.computeFamily = findQueueFamilies(m_Context.physicalDevice, VK_QUEUE_COMPUTE_BIT);
            m_DeviceQueueIndices.push_back(m_DeviceDesc.computeFamily);
        }
        VK_CHECK(createDevice(deviceFeatures ,deviceFeatures2));

        if(desc.useGraphicsQueue)
        {
            VkQueue graphicsQueue;
        	vkGetDeviceQueue(m_Context.device, m_DeviceDesc.graphicsFamily, 0, &graphicsQueue);
            m_Queues[uint32_t(CommandQueue::Graphics)] = std::make_unique<Queue>(m_Context,
                CommandQueue::Graphics, graphicsQueue, m_DeviceDesc.graphicsFamily);
        }
        if (m_Queues[uint32_t(CommandQueue::Graphics)].get() == nullptr)
        {
            exit(EXIT_FAILURE);
        }

        if(desc.useComputeQueue)
        {
            VkQueue computeQueue;
            vkGetDeviceQueue(m_Context.device, m_DeviceDesc.computeFamily, 0, &computeQueue);
            m_Queues[uint32_t(CommandQueue::Compute)] = std::make_unique<Queue>(m_Context,
                CommandQueue::Compute, computeQueue, m_DeviceDesc.computeFamily);
            if (m_Queues[uint32_t(CommandQueue::Compute)].get() == nullptr)
            {
                exit(EXIT_FAILURE);
            }
        }

        VkBool32 presentSupported = 0;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_Context.physicalDevice, m_DeviceDesc.graphicsFamily, m_Context.vulkanInstance.surface, &presentSupported);
        if (!presentSupported)
            exit(EXIT_FAILURE);

        VK_CHECK(createSwapchain(m_Context.device, m_Context.physicalDevice, m_Context.vulkanInstance.surface, m_DeviceDesc.graphicsFamily, desc.framebufferWidth, desc.framebufferHeight, &m_Resources.swapchain, m_Context.ctxFeatures.supportsScreenshots_));
        const size_t imageCount = createSwapchainImages(m_Context.device, m_Resources.swapchain, m_Resources.swapchainImages, m_Resources.swapchainImageViews);
        m_Resources.commandBuffers.resize(imageCount);

        VK_CHECK(createSemaphore(m_Context.device, &getQueue(CommandQueue::Graphics)->semaphore));
        VK_CHECK(createSemaphore(m_Context.device, &getQueue(CommandQueue::Graphics)->renderSemaphore));

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

        if(desc.useComputeQueue)
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

    VkResult Device::createDevice(VkPhysicalDeviceFeatures deviceFeatures, VkPhysicalDeviceFeatures2 deviceFeatures2)
    {
        std::vector<const char*> extensions{};
        if(m_Context.ctxExtensions.KHR_swapchain)
        {
            extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }
        if(m_Context.ctxExtensions.KHR_maintenance3)
        {
            extensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
        }
        if(m_Context.ctxExtensions.EXT_discriptor_indexing)
        {
            extensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
        }
        if(m_Context.ctxExtensions.EXT_draw_indirect_count)
        {
            // for legacy drivers Vulkan 1.1
            extensions.push_back(VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME);
        }
#if defined (__APPLE__)
        if (ctx_.ctxExtensions.KHR_portability_subset)
        {
            // for legacy drivers Vulkan 1.1
            extensions.push_back("VK_KHR_portability_subset");
        }
#endif

        if (m_DeviceDesc.graphicsFamily == m_DeviceDesc.computeFamily)
        {
            m_DeviceDesc.useComputeQueue = false;
        }

        const float queuePriorities[2] = { 0.f, 0.f };

        std::vector<VkDeviceQueueCreateInfo> qci{};

        VkDeviceQueueCreateInfo qciGfx{};
        qciGfx.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qciGfx.pNext = nullptr;
        qciGfx.flags = 0;
        qciGfx.queueFamilyIndex = m_DeviceDesc.graphicsFamily;
        qciGfx.queueCount = 1;
        qciGfx.pQueuePriorities = &queuePriorities[0];
        qci.push_back(qciGfx);

        if(m_DeviceDesc.useComputeQueue)
        {
            VkDeviceQueueCreateInfo qciComp{};
            qciComp.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            qciComp.pNext = nullptr;
            qciComp.flags = 0;
            qciComp.queueFamilyIndex = static_cast<uint32_t>(m_Queues[uint32_t(CommandQueue::Compute)]->getQueueFamilyIndex());
            qciComp.queueCount = 1;
            qciComp.pQueuePriorities = &queuePriorities[1];
            qci.push_back(qciComp);
        }

        VkDeviceCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        ci.pNext = m_Context.ctxFeatures.deviceDescriptorIndexing ? &deviceFeatures2 : nullptr;
        ci.flags = 0;
        ci.queueCreateInfoCount = static_cast<uint32_t>(qci.size());
        ci.pQueueCreateInfos = qci.data();
        ci.enabledLayerCount = 0;
        ci.ppEnabledLayerNames = nullptr;
        ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        ci.ppEnabledExtensionNames = extensions.data();
        ci.pEnabledFeatures = m_Context.ctxFeatures.deviceDescriptorIndexing ? nullptr : &deviceFeatures;

        return vkCreateDevice(m_Context.physicalDevice, &ci, nullptr, &m_Context.device);
    }

    void Device::destroyVulkanRenderDevice()
    {
        for (size_t i = 0; i < m_Resources.swapchainImages.size(); i++)
            vkDestroyImageView(m_Context.device, m_Resources.swapchainImageViews[i], nullptr);

        vkDestroySwapchainKHR(m_Context.device, m_Resources.swapchain, nullptr);

        vkDestroyCommandPool(m_Context.device, m_Resources.commandPool, nullptr);

        vkDestroySemaphore(m_Context.device, getQueue(CommandQueue::Graphics)->semaphore, nullptr);
        vkDestroySemaphore(m_Context.device, getQueue(CommandQueue::Graphics)->renderSemaphore, nullptr);

        if (m_DeviceDesc.useComputeQueue)
        {
            vkDestroyCommandPool(m_Context.device, m_Resources.computeCommandPool, nullptr);
        }

        vkDestroyDevice(m_Context.device, nullptr);
    }
}