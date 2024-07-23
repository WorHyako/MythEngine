#include <RHI/RHIVulkan/VulkanBackend.hpp>

namespace RHI::Vulkan
{
    VulkanDevice::VulkanDevice(
        VulkanInstance& vk,
        VulkanRenderDevice& vkDev,
        VulkanContextExtensions& ctxExtensions,
        VulkanContextFeatures& ctxFeatures,
        uint32_t width,
        uint32_t height)
        : ctx_(vk, vkDev, ctxExtensions, ctxFeatures)
		, resources_(vkDev)
    {
        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        VkPhysicalDeviceFeatures deviceFeatures = initVulkanRenderDeviceFeatures(ctx_.ctxFeatures, deviceFeatures2);

        initVulkanRenderDevice(width, height, isDeviceSuitable, deviceFeatures, deviceFeatures2);
    }

    VulkanDevice::~VulkanDevice()
    {
        destroyVulkanRenderDevice();
    }

    VkPhysicalDeviceFeatures VulkanDevice::initVulkanRenderDeviceFeatures(const VulkanContextFeatures& ctxFeatures, VkPhysicalDeviceFeatures2& deviceFeatures2)
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

    bool VulkanDevice::initVulkanRenderDevice(
        uint32_t width,
        uint32_t height,
        std::function<bool(VkPhysicalDevice)> selector,
        VkPhysicalDeviceFeatures deviceFeatures,
        VkPhysicalDeviceFeatures2 deviceFeatures2)
    {
        ctx_.vkDev.framebufferWidth = width;
        ctx_.vkDev.framebufferHeight = height;

        VK_CHECK(findSuitablePhysicalDevice(ctx_.vk.instance, selector, &ctx_.vkDev.physicalDevice));
        ctx_.vkDev.graphicsFamily = findQueueFamilies(ctx_.vkDev.physicalDevice, VK_QUEUE_GRAPHICS_BIT);
        //	VK_CHECK(createDevice2(vkDev.physicalDevice, deviceFeatures2, vkDev.graphicsFamily, &vkDev.device));
        //	VK_CHECK(vkGetBestComputeQueue(vkDev.physicalDevice, &vkDev.computeFamily));
        if(ctx_.vkDev.useCompute)
        {
            ctx_.vkDev.computeFamily = findQueueFamilies(ctx_.vkDev.physicalDevice, VK_QUEUE_COMPUTE_BIT);
        }
        VK_CHECK(createDevice(deviceFeatures ,deviceFeatures2));

        vkGetDeviceQueue(ctx_.vkDev.device, ctx_.vkDev.graphicsFamily, 0, &ctx_.vkDev.graphicsQueue);
        if (ctx_.vkDev.graphicsQueue == nullptr)
            exit(EXIT_FAILURE);

        if(ctx_.vkDev.useCompute)
        {
            vkGetDeviceQueue(ctx_.vkDev.device, ctx_.vkDev.computeFamily, 0, &ctx_.vkDev.computeQueue);
            if (ctx_.vkDev.computeQueue == nullptr)
                exit(EXIT_FAILURE);
        }

        VkBool32 presentSupported = 0;
        vkGetPhysicalDeviceSurfaceSupportKHR(ctx_.vkDev.physicalDevice, ctx_.vkDev.graphicsFamily, ctx_.vk.surface, &presentSupported);
        if (!presentSupported)
            exit(EXIT_FAILURE);

        VK_CHECK(createSwapchain(ctx_.vkDev.device, ctx_.vkDev.physicalDevice, ctx_.vk.surface, ctx_.vkDev.graphicsFamily, width, height, &ctx_.vkDev.swapchain, ctx_.ctxFeatures.supportsScreenshots_));
        const size_t imageCount = createSwapchainImages(ctx_.vkDev.device, ctx_.vkDev.swapchain, ctx_.vkDev.swapchainImages, ctx_.vkDev.swapchainImageViews);
        ctx_.vkDev.commandBuffers.resize(imageCount);

        VK_CHECK(createSemaphore(ctx_.vkDev.device, &ctx_.vkDev.semaphore));
        VK_CHECK(createSemaphore(ctx_.vkDev.device, &ctx_.vkDev.renderSemaphore));

        VkCommandPoolCreateInfo cpi{};
        cpi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cpi.flags = 0;
        cpi.queueFamilyIndex = ctx_.vkDev.graphicsFamily;

        VK_CHECK(vkCreateCommandPool(ctx_.vkDev.device, &cpi, nullptr, &ctx_.vkDev.commandPool));

        VkCommandBufferAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.pNext = nullptr;
        ai.commandPool = ctx_.vkDev.commandPool;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = static_cast<uint32_t>(ctx_.vkDev.swapchainImages.size());

        VK_CHECK(vkAllocateCommandBuffers(ctx_.vkDev.device, &ai, &ctx_.vkDev.commandBuffers[0]));

        if(ctx_.vkDev.useCompute)
        {
            // Create compute command pool
            VkCommandPoolCreateInfo cpi1;
            cpi1.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cpi1.pNext = nullptr;
            cpi1.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; /* Allow command from this pool buffers to be reset*/
            cpi1.queueFamilyIndex = ctx_.vkDev.computeFamily;

            VK_CHECK(vkCreateCommandPool(ctx_.vkDev.device, &cpi1, nullptr, &ctx_.vkDev.computeCommandPool));

            VkCommandBufferAllocateInfo ai1{};
            ai1.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            ai1.pNext = nullptr;
            ai1.commandPool = ctx_.vkDev.computeCommandPool;
            ai1.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            ai1.commandBufferCount = 1;

            VK_CHECK(vkAllocateCommandBuffers(ctx_.vkDev.device, &ai1, &ctx_.vkDev.computeCommandBuffer));
        }

        return true;
    }

    VkResult VulkanDevice::createDevice(VkPhysicalDeviceFeatures deviceFeatures, VkPhysicalDeviceFeatures2 deviceFeatures2)
    {
        std::vector<const char*> extensions{};
        if(ctx_.ctxExtensions.KHR_swapchain)
        {
            extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }
        if(ctx_.ctxExtensions.KHR_maintenance3)
        {
            extensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
        }
        if(ctx_.ctxExtensions.EXT_discriptor_indexing)
        {
            extensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
        }
        if(ctx_.ctxExtensions.EXT_draw_indirect_count)
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

        if (ctx_.vkDev.graphicsFamily == ctx_.vkDev.computeFamily)
        {
            ctx_.vkDev.useCompute = false;
        }

        const float queuePriorities[2] = { 0.f, 0.f };

        std::vector<VkDeviceQueueCreateInfo> qci{};

        VkDeviceQueueCreateInfo qciGfx{};
        qciGfx.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qciGfx.pNext = nullptr;
        qciGfx.flags = 0;
        qciGfx.queueFamilyIndex = ctx_.vkDev.graphicsFamily;
        qciGfx.queueCount = 1;
        qciGfx.pQueuePriorities = &queuePriorities[0];
        qci.push_back(qciGfx);

        if(ctx_.vkDev.useCompute)
        {
            VkDeviceQueueCreateInfo qciComp{};
            qciComp.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            qciComp.pNext = nullptr;
            qciComp.flags = 0;
            qciComp.queueFamilyIndex = ctx_.vkDev.computeFamily;
            qciComp.queueCount = 1;
            qciComp.pQueuePriorities = &queuePriorities[1];
            qci.push_back(qciComp);
        }

        VkDeviceCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        ci.pNext = ctx_.ctxFeatures.deviceDescriptorIndexing ? &deviceFeatures2 : nullptr;
        ci.flags = 0;
        ci.queueCreateInfoCount = static_cast<uint32_t>(qci.size());
        ci.pQueueCreateInfos = qci.data();
        ci.enabledLayerCount = 0;
        ci.ppEnabledLayerNames = nullptr;
        ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        ci.ppEnabledExtensionNames = extensions.data();
        ci.pEnabledFeatures = ctx_.ctxFeatures.deviceDescriptorIndexing ? nullptr : &deviceFeatures;

        return vkCreateDevice(ctx_.vkDev.physicalDevice, &ci, nullptr, &ctx_.vkDev.device);
    }

    void VulkanDevice::destroyVulkanRenderDevice()
    {
        for (size_t i = 0; i < ctx_.vkDev.swapchainImages.size(); i++)
            vkDestroyImageView(ctx_.vkDev.device, ctx_.vkDev.swapchainImageViews[i], nullptr);

        vkDestroySwapchainKHR(ctx_.vkDev.device, ctx_.vkDev.swapchain, nullptr);

        vkDestroyCommandPool(ctx_.vkDev.device, ctx_.vkDev.commandPool, nullptr);

        vkDestroySemaphore(ctx_.vkDev.device, ctx_.vkDev.semaphore, nullptr);
        vkDestroySemaphore(ctx_.vkDev.device, ctx_.vkDev.renderSemaphore, nullptr);

        if (ctx_.vkDev.useCompute)
        {
            vkDestroyCommandPool(ctx_.vkDev.device, ctx_.vkDev.computeCommandPool, nullptr);
        }

        vkDestroyDevice(ctx_.vkDev.device, nullptr);
    }
}