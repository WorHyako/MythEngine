#pragma once
#include <RHI/Vulkan/VulkanBaseRender.hpp>
#include <memory>

class VulkanClear;
class VulkanFinish;
class ComputedImage;
class ComputedVertexBuffer;
class ModelRenderer;
class ImGuiRenderer;

class VulkanComputeMeshRender : public VulkanBaseRender
{
public:
    VulkanComputeMeshRender(const uint32_t screenWidth, uint32_t screenHeight);
    virtual int draw() override;
    virtual bool initVulkan() override;
    virtual void terminateVulkan() override;
	virtual void recreateSwapchain() override;
    virtual void cleanupSwapchain() override;
    
    std::unique_ptr<VulkanClear> clear;
    std::unique_ptr<ComputedVertexBuffer> meshGen;
    std::unique_ptr<ModelRenderer> mesh;
    std::unique_ptr<ModelRenderer> meshColor;
    std::unique_ptr<ComputedImage> imgGen;
    std::unique_ptr<ImGuiRenderer> imgui;
    std::unique_ptr<VulkanFinish> finish;

protected:
    void composeFrame(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void updateBuffers(uint32_t imageIndex);
    void renderGUI(uint32_t imageIndex);
    void initMesh();
};