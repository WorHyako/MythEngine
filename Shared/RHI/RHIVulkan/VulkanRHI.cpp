#include <unordered_set>
#include <glm/ext/scalar_common.hpp>
#include <RHI/RHIVulkan/VulkanBackend.hpp>

#ifdef NDEBUG
const bool enableValidationLayers = false;
const bool enableValidationFeaturesEnabled = false;
const bool enableValidationFeaturesDisabled = false;
#else
const bool enableValidationLayers = true;
const bool enableValidationFeaturesEnabled = false;
const bool enableValidationFeaturesDisabled = true;
#endif

namespace RHI::Vulkan
{
    VulkanRHIModule::VulkanRHIModule()
	    : IRHIModule()
    {
        glslang_initialize_process();

        volkInitialize();

        if (!glfwInit())
        {
            exit(EXIT_FAILURE);
        }

        if (!glfwVulkanSupported())
        {
            exit(EXIT_FAILURE);
        }
    }

	IDynamicRHI* VulkanRHIModule::createRHI()
	{
		VulkanDynamicRHI* VulkanRHI = new VulkanDynamicRHI();
		return VulkanRHI;
	}

	VulkanDynamicRHI::VulkanDynamicRHI()
	{
		createInstance();

        if (!setupDebugCallbacks(m_VulkanInstance.instance, &m_VulkanInstance.messenger, &m_VulkanInstance.reportCallback))
        {
            exit(EXIT_FAILURE);
        }

        createWindowSurface();
        CreateDevice();
	}

    RHI::IDevice* VulkanDynamicRHI::getDevice() const
    {
        return m_Device;
    }

    void VulkanDynamicRHI::setWindow(GLFWwindow* window)
    {
        m_Window = window;
    }

    void VulkanDynamicRHI::createWindowSurface()
    {
        if (glfwCreateWindowSurface(m_VulkanInstance.instance, m_Window, nullptr, &m_VulkanInstance.surface) != VK_SUCCESS)
        {
            exit(EXIT_FAILURE);
        }
    }

	VulkanDynamicRHI::~VulkanDynamicRHI()
	{
        destroyVulkanInstance();
	}

	bool IsExtensionAvailable(const std::vector<VkExtensionProperties>& properties,
		const char* extension) noexcept {
		for (const VkExtensionProperties& p : properties) {
			if (strcmp(p.extensionName, extension) == 0) {
				return true;
			}
		}
		return false;
	}

    VulkanContextExtensions& VulkanDynamicRHI::initializeContextExtensions()
    {
        VulkanContextExtensions contextExtensions{
            .KHR_swapchain = true,
            .KHR_maintenance3 = true,
            .EXT_discriptor_indexing = true,
            .EXT_draw_indirect_count = true,
#if defined(__APPLE__)
            .KHR_portability_subset = true
#endif
        };
        return contextExtensions;
    }

    VulkanContextFeatures& VulkanDynamicRHI::initializeContextFeatures()
    {
        VulkanContextFeatures contextFeatures{
            .supportsScreenshots_ = true,
            .geometryShader_ = true,
            .tessellationShader_ = true,
            .multiDrawIndirect = true,
            .drawIndirectFirstInstance = true,
            .vertexPipelineStoresAndAtomics_ = true,
            .fragmentStoresAndAtomics_ = true,
            .shaderSampledImageArrayDynamicIndexing = true,
            .shaderInt64 = true,
            .deviceDescriptorIndexing = true
        };
        return contextFeatures;
    }

    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        const bool isDiscreteGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        const bool isIntegratedGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
        const bool isGPU = isDiscreteGPU || isIntegratedGPU;

#if defined(WIN32)
        return isGPU && deviceFeatures.geometryShader;
#elif defined(__APPLE__)
        return isGPU;
#endif
        return false;
    }

    void VulkanDynamicRHI::CreateDevice()
    {
        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        VkPhysicalDeviceFeatures deviceFeatures = initVulkanRenderDeviceFeatures(m_VulkanFeatures, deviceFeatures2);

        VK_CHECK(findSuitablePhysicalDevice(m_VulkanInstance.instance, isDeviceSuitable, &m_VulkanPhysicalDevice));

        std::unordered_set<uint32_t> uniqueQueueFamilies{};
        if (m_DeviceParams.useGraphicsQueue)
        {
            m_GraphicsQueueFamily = findQueueFamilies(m_VulkanPhysicalDevice, VK_QUEUE_GRAPHICS_BIT);
            uniqueQueueFamilies.insert(m_GraphicsQueueFamily);
        }
        //	VK_CHECK(createDevice2(m_Context.m_PhysicalDevice, deviceFeatures2, vkDev.graphicsFamily, &m_Context.m_Device));
        //	VK_CHECK(vkGetBestComputeQueue(m_Context.m_PhysicalDevice, &vkDev.computeFamily));
        if (m_DeviceParams.useComputeQueue)
        {
            m_ComputeQueueFamily = findQueueFamilies(m_VulkanPhysicalDevice, VK_QUEUE_COMPUTE_BIT);
            uniqueQueueFamilies.insert(m_ComputeQueueFamily);
        }

        if (m_DeviceParams.useTransferQueue)
        {
            m_TransferQueueFamily = findQueueFamilies(m_VulkanPhysicalDevice, VK_QUEUE_TRANSFER_BIT);
            uniqueQueueFamilies.insert(m_TransferQueueFamily);
        }

    	VK_CHECK(createDevice(deviceFeatures, deviceFeatures2));

        RHI::Vulkan::DeviceDesc DeviceDesc = {
            .framebufferWidth = m_DeviceParams.backBufferWidth,
            .framebufferHeight = m_DeviceParams.backBufferHeight,
            .instance = m_VulkanInstance.instance,
            .physicalDevice = m_VulkanPhysicalDevice,
            .device = m_VulkanDevice,
            .ctxExtensions = &m_VulkanExtensions,
            .ctxFeatures = &m_VulkanFeatures,
            .graphicsFamily = m_GraphicsQueueFamily,
            .graphicsQueue = m_GraphicsQueue,
            .useGraphicsQueue = m_DeviceParams.useGraphicsQueue,
            .computeFamily = m_ComputeQueueFamily,
            .computeQueue = m_ComputeQueue,
            .useComputeQueue = m_DeviceParams.useComputeQueue,
            .transferFamily = m_TransferQueueFamily,
            .transferQueue = m_TransferQueue,
            .useTransferQueue = m_DeviceParams.useTransferQueue };

        m_Device = new RHI::Vulkan::Device(DeviceDesc);

        if (m_DeviceParams.useGraphicsQueue)
        {
            vkGetDeviceQueue(m_VulkanDevice, m_GraphicsQueueFamily, 0, &m_GraphicsQueue);
        }
        if (m_GraphicsQueue == nullptr)
        {
            exit(EXIT_FAILURE);
        }

        if (m_DeviceParams.useComputeQueue)
        {
            vkGetDeviceQueue(m_VulkanDevice, m_ComputeQueueFamily, 0, &m_ComputeQueue);
            if (m_ComputeQueue == nullptr)
            {
                exit(EXIT_FAILURE);
            }
        }

        VkBool32 presentSupported = 0;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_VulkanPhysicalDevice, m_GraphicsQueueFamily, m_VulkanInstance.surface, &presentSupported);
        if (!presentSupported)
        {
            exit(EXIT_FAILURE);
        }

        VK_CHECK(createSwapchain());
        const size_t imageCount = createSwapchainImages();
        m_SwapChainIndex = 0;

        m_PresentSemaphores.reserve(m_DeviceParams.maxFramesInFlight + 1);
        m_AcquireSemaphores.reserve(m_DeviceParams.maxFramesInFlight + 1);
        const VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        for (uint32_t i = 0; i < m_DeviceParams.maxFramesInFlight + 1; ++i)
        {
            vkCreateSemaphore(m_VulkanDevice, &semaphoreCreateInfo, nullptr, &m_PresentSemaphores[i]);
            vkCreateSemaphore(m_VulkanDevice, &semaphoreCreateInfo, nullptr, &m_AcquireSemaphores[i]);
        }
    }

    VkResult VulkanDynamicRHI::createDevice(VkPhysicalDeviceFeatures deviceFeatures, VkPhysicalDeviceFeatures2 deviceFeatures2)
    {
        std::vector<const char*> extensions{};
        if (m_VulkanExtensions.KHR_swapchain)
        {
            extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }
        if (m_VulkanExtensions.KHR_maintenance3)
        {
            extensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
        }
        if (m_VulkanExtensions.EXT_discriptor_indexing)
        {
            extensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
        }
        if (m_VulkanExtensions.EXT_draw_indirect_count)
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

        if (m_GraphicsQueueFamily == m_ComputeQueueFamily)
        {
            m_DeviceParams.useComputeQueue = false;
        }

        const float queuePriorities[2] = { 0.f, 0.f };

        std::vector<VkDeviceQueueCreateInfo> qci{};

        VkDeviceQueueCreateInfo qciGfx{};
        qciGfx.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qciGfx.pNext = nullptr;
        qciGfx.flags = 0;
        qciGfx.queueFamilyIndex = m_GraphicsQueueFamily;
        qciGfx.queueCount = 1;
        qciGfx.pQueuePriorities = &queuePriorities[0];
        qci.push_back(qciGfx);

        if (m_DeviceParams.useComputeQueue)
        {
            VkDeviceQueueCreateInfo qciComp{};
            qciComp.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            qciComp.pNext = nullptr;
            qciComp.flags = 0;
            qciComp.queueFamilyIndex = m_ComputeQueueFamily;
            qciComp.queueCount = 1;
            qciComp.pQueuePriorities = &queuePriorities[1];
            qci.push_back(qciComp);
        }

        VkDeviceCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        ci.pNext = m_VulkanFeatures.deviceDescriptorIndexing ? &deviceFeatures2 : nullptr;
        ci.flags = 0;
        ci.queueCreateInfoCount = static_cast<uint32_t>(qci.size());
        ci.pQueueCreateInfos = qci.data();
        ci.enabledLayerCount = 0;
        ci.ppEnabledLayerNames = nullptr;
        ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        ci.ppEnabledExtensionNames = extensions.data();
        ci.pEnabledFeatures = m_VulkanFeatures.deviceDescriptorIndexing ? nullptr : &deviceFeatures;

        return vkCreateDevice(m_VulkanPhysicalDevice, &ci, nullptr, &m_VulkanDevice);
    }

    GraphicsAPI VulkanDynamicRHI::getGraphicsAPI() const
    {
        return GraphicsAPI::VULKAN;
    }

    void VulkanDynamicRHI::destroySwapChain()
    {
        for (size_t i = 0; i < m_SwapchainImages.size(); i++)
        {
            vkDestroyImageView(m_VulkanDevice, m_SwapchainImageViews[i], nullptr);
        }
        m_SwapchainImageViews.clear();

        if(m_SwapChain)
        {
            vkDestroySwapchainKHR(m_VulkanDevice, m_SwapChain, nullptr);
            m_SwapChain = nullptr;
        }
    }



    bool VulkanDynamicRHI::createSwapchain()
    {
        destroySwapChain();

        auto swapchainSupport = querySwapchainSupport(m_VulkanPhysicalDevice, m_VulkanInstance.surface);
        auto surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
        auto presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);

        const VkSwapchainCreateInfoKHR ci = {
                VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                nullptr,
                0,
                m_VulkanInstance.surface,
                chooseSwapImageCount(swapchainSupport.capabilities),
                surfaceFormat.format,
                surfaceFormat.colorSpace,
                {m_DeviceParams.backBufferWidth, m_DeviceParams.backBufferHeight},
                1,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | (m_DeviceParams.supportScreenshots ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0u),
                VK_SHARING_MODE_EXCLUSIVE,
                1,
                &m_GraphicsQueueFamily,
                swapchainSupport.capabilities.currentTransform,
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                presentMode,
                VK_TRUE,
                VK_NULL_HANDLE };

        return vkCreateSwapchainKHR(m_VulkanDevice, &ci, nullptr, &m_SwapChain);
    }

    size_t VulkanDynamicRHI::createSwapchainImages()
    {
        uint32_t imageCount = 0;
        VK_CHECK(vkGetSwapchainImagesKHR(m_VulkanDevice, m_SwapChain, &imageCount, nullptr));

        m_SwapchainImages.resize(imageCount);
        m_SwapchainImageViews.resize(imageCount);

        VK_CHECK(vkGetSwapchainImagesKHR(m_VulkanDevice, m_SwapChain, &imageCount, m_SwapchainImages.data()));

        for (unsigned i = 0; i < imageCount; i++)
            if (!createImageView(m_VulkanDevice, m_SwapchainImages[i], VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &m_SwapchainImageViews[i]))
                exit(0);

        return static_cast<size_t>(imageCount);
    }

    void VulkanDynamicRHI::resizeSwapchain()
    {
        if (m_VulkanDevice)
        {
            destroySwapChain();
            createSwapchain();
        }
    }

    VkPhysicalDeviceFeatures VulkanDynamicRHI::initVulkanRenderDeviceFeatures(const VulkanContextFeatures& ctxFeatures, VkPhysicalDeviceFeatures2& deviceFeatures2)
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

        if (ctxFeatures.deviceDescriptorIndexing)
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

	void VulkanDynamicRHI::createInstance()
	{
        const std::vector<const char*> validationLayers = {
                "VK_LAYER_KHRONOS_validation"
        };

        // Enumerate available extensions
        std::uint32_t propertiesCount;
        std::vector<VkExtensionProperties> properties;
        vkEnumerateInstanceExtensionProperties(nullptr, &propertiesCount, nullptr);
        properties.resize(static_cast<int>(propertiesCount));
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &propertiesCount, properties.data()));

        std::vector<const char*> exts = {
                "VK_KHR_surface",
    #if defined(_WIN32)
                "VK_KHR_win32_surface",
    #endif
    #if defined (__APPLE__)
                "VK_MVK_macos_surface",
                VK_EXT_LAYER_SETTINGS_EXTENSION_NAME,
                VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
    #endif
    #if defined (__linux__)
                "VK_KHR_xcb_surface"
    #endif
                //, VK_EXT_DEBUG_UTILS_EXTENSION_NAME
                //, VK_EXT_DEBUG_REPORT_EXTENSION_NAME
                /*for indexed textures*/
                //VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
        };

        uint32_t extensionsCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionsCount);
        for (uint32_t i = 0; i < extensionsCount; i++) {
            exts.push_back(glfwExtensions[i]);
        }
        // Enable required extensions
        if (IsExtensionAvailable(properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
            exts.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }

        if (enableValidationLayers)
        {
            exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            exts.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        const VkValidationFeatureEnableEXT validationFeaturesEnabled[] = {
                VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
                VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
        };

#if defined(__APPLE__)
        // Shader validation doesn't work in MoltenVK for SPIR-V 1.6 under Vulkan 1.3:
        // "Invalid SPIR-V binary version 1.6 for target environment SPIR-V 1.5 (under Vulkan 1.2 semantics)."
        const VkValidationFeatureDisableEXT validationFeaturesDisabled[] = {
                VK_VALIDATION_FEATURE_DISABLE_SHADERS_EXT,
                VK_VALIDATION_FEATURE_DISABLE_SHADER_VALIDATION_CACHE_EXT,
        };
#endif
        const VkValidationFeaturesEXT features = {
                VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
                nullptr,
                enableValidationFeaturesEnabled ? (uint32_t)(sizeof(validationFeaturesEnabled) / sizeof(VkValidationFeatureEnableEXT)) : 0u,
                enableValidationFeaturesEnabled ? validationFeaturesEnabled : nullptr,
    #if defined(_WIN32)
                0u,
            nullptr
    #endif
    #if defined(__APPLE__)
                enableValidationFeaturesDisabled ? (uint32_t)(sizeof(validationFeaturesDisabled) / sizeof(VkValidationFeatureDisableEXT)) : 0u,
                enableValidationFeaturesDisabled ? validationFeaturesDisabled : nullptr
    #endif
        };

#if defined(__APPLE__)
        // https://github.com/KhronosGroup/MoltenVK/blob/main/Docs/MoltenVK_Configuration_Parameters.md
        const int useMetalArgumentBuffers = 1;
        const VkLayerSettingEXT settings[] = {
                {"MoltenVK", "MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", VK_LAYER_SETTING_TYPE_INT32_EXT, 1, &useMetalArgumentBuffers} };
        const VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo = {
                VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
                enableValidationLayers ? &features : nullptr,
                (uint32_t)(sizeof(settings) / sizeof(VkLayerSettingEXT)),
                settings
        };
#endif

        const VkApplicationInfo appInfo = {
                VK_STRUCTURE_TYPE_APPLICATION_INFO,
                nullptr,
                "Vulkan",
                VK_MAKE_VERSION(1, 0, 0),
                "No Engine",
                VK_MAKE_VERSION(1, 0, 0),
                VK_API_VERSION_1_3
        };

        VkInstanceCreateInfo createInfo = {
                VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    #if defined(__APPLE__)
                &layerSettingsCreateInfo,
    #else
                enableValidationLayers ? &features : nullptr,
    #endif
                0,
                &appInfo,
                enableValidationLayers ? static_cast<uint32_t>(validationLayers.size()) : 0u,
                enableValidationLayers ? validationLayers.data() : nullptr,
                static_cast<uint32_t>(exts.size()),
                exts.data()
        };

#if defined (__APPLE__)
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
            exts.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
#endif
#endif

        VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_VulkanInstance.instance));

        volkLoadInstance(m_VulkanInstance.instance);
    }

    void VulkanDynamicRHI::destroyDevice()
    {
        destroySwapChain();

        //vkDestroyCommandPool(m_VulkanDevice, m_C, nullptr);

        for(VkSemaphore& semaphore : m_PresentSemaphores)
        {
	        if(semaphore)
	        {
                vkDestroySemaphore(m_VulkanDevice, semaphore, nullptr);
                semaphore = nullptr;
	        }
        }

        for (VkSemaphore& semaphore : m_AcquireSemaphores)
        {
            if (semaphore)
            {
                vkDestroySemaphore(m_VulkanDevice, semaphore, nullptr);
                semaphore = nullptr;
            }
        }

        /*if (m_DeviceParams.useComputeQueue)
        {
            vkDestroyCommandPool(m_VulkanDevice, m_Resources.computeCommandPool, nullptr);
        }*/

        m_Device = nullptr;

        vkDestroyDevice(m_VulkanDevice, nullptr);
    }

    void VulkanDynamicRHI::destroyVulkanInstance()
    {
        vkDestroySurfaceKHR(m_VulkanInstance.instance, m_VulkanInstance.surface, nullptr);

        vkDestroyDebugReportCallbackEXT(m_VulkanInstance.instance, m_VulkanInstance.reportCallback, nullptr);
        vkDestroyDebugUtilsMessengerEXT(m_VulkanInstance.instance, m_VulkanInstance.messenger, nullptr);

        vkDestroyInstance(m_VulkanInstance.instance, nullptr);
    }

    bool VulkanDynamicRHI::BeginFrame()
    {
        const auto& semaphore = m_AcquireSemaphores[m_AcquireSemaphoreIndex];

        VkResult result;

        int const maxAttempts = 3;
        for(int attempt = 0; attempt < maxAttempts; ++attempt)
        {
            result = vkAcquireNextImageKHR(m_VulkanDevice, m_SwapChain, 0, semaphore, VK_NULL_HANDLE, &m_SwapChainIndex);

            if (result == VkResult::VK_ERROR_OUT_OF_DATE_KHR && attempt < maxAttempts)
            {
                //BackBufferResizing();
                VkSurfaceCapabilitiesKHR surfaceCaps;
                vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_VulkanPhysicalDevice, m_VulkanInstance.surface, &surfaceCaps);

                m_DeviceParams.backBufferWidth = surfaceCaps.currentExtent.width;
                m_DeviceParams.backBufferHeight = surfaceCaps.currentExtent.height;

                resizeSwapchain();
                //BackBufferResized();
            }
            else
                break;
        }

        m_AcquireSemaphoreIndex = (m_AcquireSemaphoreIndex + 1) % m_AcquireSemaphores.size();

        if (result == VkResult::VK_SUCCESS)
        {
            // Schedule the wait. The actual wait operation will be submitted when the app executes any command list.
            m_Device->queueWaitForSemaphore(RHI::CommandQueue::Graphics, semaphore, 0);
            return true;
        }

        return false;
    }

    bool VulkanDynamicRHI::Present()
    {
        const auto& semaphore = m_PresentSemaphores[m_PresentSemaphoreIndex];

        VkPresentInfoKHR pi{};
        pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        pi.pNext = nullptr;
        pi.waitSemaphoreCount = 1;
        pi.pWaitSemaphores = &semaphore;
        pi.swapchainCount = 1;
        pi.pSwapchains = &m_SwapChain;
        pi.pImageIndices = &m_SwapChainIndex;

        m_PresentSemaphoreIndex = (m_PresentSemaphoreIndex + 1) % m_PresentSemaphores.size();

        VK_CHECK(vkQueuePresentKHR(m_PresetnQueue, &pi));
        VK_CHECK(vkDeviceWaitIdle(m_VulkanDevice));

        return true;
    }
}
