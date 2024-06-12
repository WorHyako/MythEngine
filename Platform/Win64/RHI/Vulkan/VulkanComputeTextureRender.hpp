#pragma once
#include <RHI/Vulkan/VulkanBaseRender.hpp>
#include <memory>

class VulkanSingleQuadRenderer;
class VulkanClear;
class VulkanFinish;
class ComputedImage;

class VulkanComputeTextureRender : public VulkanBaseRender
{
public:
    VulkanComputeTextureRender(const uint32_t screenWidth, uint32_t screenHeight);
    virtual int draw() override;
    virtual bool initVulkan() override;
    virtual void terminateVulkan() override;

    std::unique_ptr<VulkanSingleQuadRenderer> QuadRenderer;
    std::unique_ptr<VulkanClear> Clear;
    std::unique_ptr<VulkanFinish> Finish;

    std::unique_ptr<ComputedImage> imgGen;

protected:
    void composeFrame(VkCommandBuffer commandBuffer, uint32_t imageIndex);
};