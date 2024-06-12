#pragma once
#include <RHI/Vulkan/VulkanBaseRender.hpp>
#include <RHI/Vulkan/UtilsVulkan.hpp>
#include <memory>

class GLFWwindow;
class GLFWUserInput;
class VulkanQuadRenderer;
class VulkanClear;
class VulkanFinish;

class VulkanNonuniformIndexingRender : public VulkanBaseRender
{
public:
    VulkanNonuniformIndexingRender(const uint32_t screenWidth, uint32_t screenHeight);

    virtual bool initVulkan() override;
    virtual void terminateVulkan() override;
    virtual int draw() override;

    std::unique_ptr<VulkanClear> Clear;
    std::unique_ptr<VulkanFinish> Finish;
    std::unique_ptr<VulkanQuadRenderer> QuadRenderer;

protected:
    void fillQuadsBuffer(VulkanRenderDevice& vkDev, VulkanQuadRenderer& quadRenderer, size_t currentImage);
    void composeFrame(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    
private:
    GLFWUserInput* input_;

    VulkanInstance vk_;
    VulkanRenderDevice vkDev;
};