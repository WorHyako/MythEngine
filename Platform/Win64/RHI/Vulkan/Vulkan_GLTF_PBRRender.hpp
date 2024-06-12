#pragma once
#include <RHI/Vulkan/VulkanBaseRender.hpp>
#include <memory>

class VulkanClear;
class VulkanFinish;
class PBRModelRenderer;
class CameraPositioner_FirstPerson;
class TestCamera;

class Vulkan_GLTF_PBRRender : public VulkanBaseRender
{
public:
    Vulkan_GLTF_PBRRender(const uint32_t screenWidth, uint32_t screenHeight);
    virtual int draw() override;
    virtual bool initVulkan() override;
    virtual void terminateVulkan() override;

    void updateBuffers(uint32_t imageIndex);
    void composeFrame(VkCommandBuffer commandBuffer, uint32_t imageIndex);

public:
    std::unique_ptr<VulkanClear> clear;
    std::unique_ptr<VulkanFinish> finish;
    std::unique_ptr<PBRModelRenderer> modelPBR;

    VulkanImage depthTexture;

private:
    CameraPositioner_FirstPerson* positioner_;
    TestCamera* camera_;
};