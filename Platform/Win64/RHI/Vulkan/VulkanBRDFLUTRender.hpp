#pragma once
#include <RHI/Vulkan/VulkanBaseRender.hpp>
#include <memory>

class VulkanBRDFLUTRender : public VulkanBaseRender
{
public:
    VulkanBRDFLUTRender(const uint32_t screenWidth, uint32_t screenHeight);
    virtual int draw() override;
    virtual bool initVulkan() override;
    virtual void terminateVulkan() override;

protected:
    void calculateLUT(float* output);
};