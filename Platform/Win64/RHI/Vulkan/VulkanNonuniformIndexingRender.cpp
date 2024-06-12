#include <RHI/Vulkan/VulkanNonuniformIndexingRender.hpp>

#include <UserInput/GLFW/GLFWUserInput.hpp>
#include <RHI/Vulkan/Framework/VulkanApp.hpp>
#include <RHI/Vulkan/Renderers/VulkanClear.hpp>
#include <RHI/Vulkan/Renderers/VulkanQuadRenderer.hpp>
#include <RHI/Vulkan/Renderers/VulkanFinish.hpp>
#include <Filesystem/FilesystemUtilities.hpp>

const double kAnimationFPS = 60.0;
const uint32_t kNumFlipbookFrames = 100;

struct AnimationState
{
    vec2 position = vec2(0);
    double startTime = 0;
    uint32_t textureIndex = 0;
    uint32_t flipbookOffset = 0;
};

std::vector<AnimationState> animations;

void updateAnimations()
{
    for (size_t i = 0; i < animations.size();)
    {
        animations[i].textureIndex = animations[i].flipbookOffset + (uint32_t)(kAnimationFPS * ((glfwGetTime() - animations[i].startTime)));
        if (animations[i].textureIndex - animations[i].flipbookOffset > (kNumFlipbookFrames - 1))
            animations.erase(animations.begin() + i);
        else
            i++;
    }
}

void VulkanNonuniformIndexingRender::fillQuadsBuffer(VulkanRenderDevice& vkDev, VulkanQuadRenderer& quadRenderer, size_t currentImage)
{
    const float aspect = (float)vkDev.framebufferWidth / (float)vkDev.framebufferHeight;
    const float quadSize = 0.5f;

    quadRenderer.clear();
    quadRenderer.quad(-quadSize, -quadSize * aspect, quadSize, quadSize * aspect);
    quadRenderer.updateBuffer(vkDev, currentImage);
}

bool VulkanNonuniformIndexingRender::initVulkan()
{
    createInstance(&vk_.instance);

    if (!setupDebugCallbacks(vk_.instance, &vk_.messenger, &vk_.reportCallback))
        exit(EXIT_FAILURE);

    if (glfwCreateWindowSurface(vk_.instance, window_, nullptr, &vk_.surface))
        exit(EXIT_FAILURE);

    VkPhysicalDeviceDescriptorIndexingFeaturesEXT physicalDeviceDescriptorIndexingFeature{};
    physicalDeviceDescriptorIndexingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    physicalDeviceDescriptorIndexingFeature.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    physicalDeviceDescriptorIndexingFeature.descriptorBindingVariableDescriptorCount = VK_TRUE;
    physicalDeviceDescriptorIndexingFeature.runtimeDescriptorArray = VK_TRUE;

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.shaderSampledImageArrayDynamicIndexing = VK_TRUE;

    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &physicalDeviceDescriptorIndexingFeature;
    deviceFeatures2.features = deviceFeatures;

    if (!initVulkanRenderDevice2(vk_, vkDev, kScreenWidth, kScreenHeight, isDeviceSuitable, deviceFeatures2))
        exit(EXIT_FAILURE);

    std::vector<std::string> textureFiles;
    for (uint32_t j = 0; j < 3; j++)
    {
        for (uint32_t i = 0; i != kNumFlipbookFrames; i++)
        {
            char fname[1024];
            snprintf(fname, sizeof(fname), (FilesystemUtilities::GetResourcesDir() + "textures/Explosion/explosion%01u/explosion%02u-frame%03u.tga").c_str(), j, j, i + 1);
            textureFiles.push_back(fname);
        }
    }

    QuadRenderer = std::make_unique<VulkanQuadRenderer>(vkDev, textureFiles);

    for (size_t i = 0; i < vkDev.swapchainImages.size(); i++)
        fillQuadsBuffer(vkDev, *QuadRenderer.get(), i);

    VulkanImage nullTexture{};
    nullTexture.image = VK_NULL_HANDLE;
    nullTexture.imageView = VK_NULL_HANDLE;

    Clear = std::make_unique<VulkanClear>(vkDev, nullTexture);
    Finish = std::make_unique<VulkanFinish>(vkDev, nullTexture);

    return VK_SUCCESS;
}

void VulkanNonuniformIndexingRender::terminateVulkan()
{
    Finish = nullptr;
    Clear = nullptr;
    QuadRenderer = nullptr;

    destroyVulkanRenderDevice(vkDev);
    destroyVulkanInstance(vk_);
}

void VulkanNonuniformIndexingRender::composeFrame(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    updateAnimations();

    Clear->fillCommandBuffer(commandBuffer, imageIndex);

    for (size_t i = 0; i < animations.size(); i++)
    {
        QuadRenderer->pushConstants(commandBuffer, animations[i].textureIndex, animations[i].position);
        QuadRenderer->fillCommandBuffer(commandBuffer, imageIndex);
    }

    Finish->fillCommandBuffer(commandBuffer, imageIndex);
}

VulkanNonuniformIndexingRender::VulkanNonuniformIndexingRender(const uint32_t screenWidth, uint32_t screenHeight)
	: VulkanBaseRender(screenWidth, screenHeight)
{
    glslang_initialize_process();

    volkInitialize();

    if (!glfwInit())
        exit(EXIT_FAILURE);

    if (!glfwVulkanSupported())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    window_ = glfwCreateWindow(kScreenWidth, kScreenHeight, "VulkanApp", nullptr, nullptr);
    if (!window_)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    input_ = &GLFWUserInput::GetInput();
    input_->mousebutton_Callback = [this](double X, double Y) {
        const float mx = ((float)X / (float)vkDev.framebufferWidth) * 2.0f - 1.0f;
        const float my = ((float)Y / (float)vkDev.framebufferHeight) * 2.0f - 1.0f;

        AnimationState animState{};
        animState.position = vec2(mx, my);
        animState.startTime = glfwGetTime();
        animState.textureIndex = 0;
        animState.flipbookOffset = kNumFlipbookFrames * (uint32_t)(rand() % 3);
        animations.push_back(animState);
        };
}

int VulkanNonuniformIndexingRender::draw()
{
    initVulkan();

    printf("Textures loaded. Click to make an explosion.\n");

    while (!glfwWindowShouldClose(window_))
    {
        drawFrame(vkDev, [](uint32_t) {}, [=](VkCommandBuffer commandBuffer, uint32_t imageIndex) { this->composeFrame(commandBuffer, imageIndex); });
        glfwPollEvents();
    }

    terminateVulkan();

    glfwTerminate();

    glslang_finalize_process();

    return 0;
}