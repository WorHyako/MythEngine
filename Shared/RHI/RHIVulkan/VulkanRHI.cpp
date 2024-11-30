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
            exit(EXIT_FAILURE);

        if (!glfwVulkanSupported())
            exit(EXIT_FAILURE);
    }

	IDynamicRHI* VulkanRHIModule::createRHI()
	{
		VulkanDynamicRHI* VulkanRHI = new VulkanDynamicRHI(getWindowInterface());
		return VulkanRHI;
	}

	VulkanDynamicRHI::VulkanDynamicRHI(GLFWwindow* window)
		: window_(window)
	{
		createInstance();

        if (!setupDebugCallbacks(vk.instance, &vk.messenger, &vk.reportCallback))
            exit(EXIT_FAILURE);

        if (glfwCreateWindowSurface(vk.instance, window_, nullptr, &vk.surface) != VK_SUCCESS)
            exit(EXIT_FAILURE);

        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);

        DeviceDesc desc = {};
        desc.ctxExtensions = &initializeContextExtensions();
        desc.ctxFeatures = &initializeContextFeatures();
        m_Device = new Device(desc);
	}

	VulkanDynamicRHI::~VulkanDynamicRHI()
	{
        destroyVulkanInstance(vk);
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

        VK_CHECK(vkCreateInstance(&createInfo, nullptr, &vk.instance));

        volkLoadInstance(vk.instance);
    }

    void VulkanDynamicRHI::destroyVulkanInstance(VulkanInstance& vk)
    {
        vkDestroySurfaceKHR(vk.instance, vk.surface, nullptr);

        vkDestroyDebugReportCallbackEXT(vk.instance, vk.reportCallback, nullptr);
        vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.messenger, nullptr);

        vkDestroyInstance(vk.instance, nullptr);
    }
}