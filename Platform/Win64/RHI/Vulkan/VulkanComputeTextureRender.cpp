#include <RHI/Vulkan/VulkanComputeTextureRender.hpp>

#include <UserInput/GLFW/GLFWUserInput.hpp>
#include <RHI/Vulkan/Framework/VulkanApp.hpp>
#include <RHI/Vulkan/Renderers/VulkanClear.hpp>
#include <RHI/Vulkan/Renderers/VulkanSingleQuad.hpp>
#include <RHI/Vulkan/Renderers/VulkanFinish.hpp>
#include <Filesystem/FilesystemUtilities.hpp>

#include "Renderers/VulkanComputedImage.hpp"

bool VulkanComputeTextureRender::initVulkan()
{
    createInstance(&vk_.instance);

    if (!setupDebugCallbacks(vk_.instance, &vk_.messenger, &vk_.reportCallback))
        exit(EXIT_FAILURE);

    const auto createSurface = glfwCreateWindowSurface(vk_.instance, window_, nullptr, &vk_.surface);
    if(createSurface != VK_SUCCESS)
        exit(EXIT_FAILURE);

    if (!initVulkanRenderDeviceWithCompute(vk_, vkDev, kScreenWidth, kScreenHeight, VkPhysicalDeviceFeatures{}))
        exit(EXIT_FAILURE);

    VulkanImage nullTexture{};
    nullTexture.image = VK_NULL_HANDLE;
    nullTexture.imageView = VK_NULL_HANDLE;

    Clear = std::make_unique<VulkanClear>(vkDev, nullTexture);
    Finish = std::make_unique<VulkanFinish>(vkDev, nullTexture);
    imgGen = std::make_unique<ComputedImage>(vkDev, (FilesystemUtilities::GetShadersDir() + "Vulkan/VK03_Compute_Texture.comp").c_str(), 1024, 1024, false);
    QuadRenderer = std::make_unique<VulkanSingleQuadRenderer>(vkDev, imgGen->computed, imgGen->computedImageSampler);

    return VK_SUCCESS;
}

void VulkanComputeTextureRender::terminateVulkan()
{
    destroyVulkanImage(vkDev.device, imgGen->computed);
    vkDestroySampler(vkDev.device, imgGen->computedImageSampler, nullptr);

    Finish = nullptr;
    Clear = nullptr;
    QuadRenderer = nullptr;
    imgGen = nullptr;

    destroyVulkanRenderDevice(vkDev);
    destroyVulkanInstance(vk_);
}

void VulkanComputeTextureRender::composeFrame(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    Clear->fillCommandBuffer(commandBuffer, imageIndex);

    insertComputedImageBarrier(commandBuffer, imgGen->computed.image);

    QuadRenderer->fillCommandBuffer(commandBuffer, imageIndex);
    Finish->fillCommandBuffer(commandBuffer, imageIndex);
}

VulkanComputeTextureRender::VulkanComputeTextureRender(const uint32_t screenWidth, uint32_t screenHeight)
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
}

int VulkanComputeTextureRender::draw()
{
    initVulkan();

    while (!glfwWindowShouldClose(window_))
    {
        float thisTime = (float)glfwGetTime();
        imgGen->fillComputeCommandBuffer(&thisTime, sizeof(float), imgGen->computedWidth / 16, imgGen->computedHeight / 16, 1);
        imgGen->submit();
        vkDeviceWaitIdle(vkDev.device);

        drawFrame(vkDev, [](uint32_t) {}, [=](VkCommandBuffer commandBuffer, uint32_t imageIndex) { this->composeFrame(commandBuffer, imageIndex); });
        glfwPollEvents();

        vkDeviceWaitIdle(vkDev.device);
    }

    imgGen->waitFence();

    terminateVulkan();

    glfwTerminate();

    glslang_finalize_process();

    return 0;
}