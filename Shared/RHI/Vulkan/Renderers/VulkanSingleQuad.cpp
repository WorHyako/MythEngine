#include <RHI/Vulkan/Renderers/VulkanSingleQuad.hpp>
#include <Filesystem/FilesystemUtilities.hpp>
#include <stdio.h>

bool VulkanSingleQuadRenderer::createDescriptorSet(VulkanRenderDevice& vkDev, VkImageLayout desiredLayout)
{
    VkDescriptorSetLayoutBinding binding =
        descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = nullptr;
    layoutInfo.flags = 0;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;

    VK_CHECK(vkCreateDescriptorSetLayout(vkDev.device, &layoutInfo, nullptr, &descriptorSetLayout_));

    const std::vector<VkDescriptorSetLayout> layouts(vkDev.swapchainImages.size(), descriptorSetLayout_);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(vkDev.swapchainImages.size());
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets_.resize(vkDev.swapchainImages.size());

    VK_CHECK(vkAllocateDescriptorSets(vkDev.device, &allocInfo, descriptorSets_.data()));

    VkDescriptorImageInfo textureDescriptor{};
    textureDescriptor.sampler = textureSampler;
    textureDescriptor.imageView = texture.imageView;
    textureDescriptor.imageLayout = desiredLayout;

    for (size_t i = 0; i < vkDev.swapchainImages.size(); i++)
    {
        VkWriteDescriptorSet imageDescriptorWrite{};
        imageDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        imageDescriptorWrite.dstSet = descriptorSets_[i];
        imageDescriptorWrite.dstBinding = 0;
        imageDescriptorWrite.dstArrayElement = 0;
        imageDescriptorWrite.descriptorCount = 1;
        imageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        imageDescriptorWrite.pImageInfo = &textureDescriptor;

        vkUpdateDescriptorSets(vkDev.device, 1, &imageDescriptorWrite, 0, nullptr);
    }

    return true;
}

void VulkanSingleQuadRenderer::fillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage)
{
    beginRenderPass(commandBuffer, currentImage);

    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
}

VulkanSingleQuadRenderer::VulkanSingleQuadRenderer(VulkanRenderDevice& vkDev, VulkanImage tex, VkSampler sampler, VkImageLayout desiredLayout)
    : vkDev(vkDev)
    , texture(tex)
    , textureSampler(sampler)
    , RendererBase(vkDev, VulkanImage())
{
    /* we don't need them, but allocate them to allow destructor to complete */
    if (!createUniformBuffers(vkDev, sizeof(uint32_t)) ||
        !createDescriptorPool(vkDev, 0, 0, 1, &descriptorPool_) ||
        !createDescriptorSet(vkDev, desiredLayout) ||
        !createColorAndDepthRenderPass(vkDev, false, &renderPass_, RenderPassCreateInfo()) ||
        !createPipelineLayout(vkDev.device, descriptorSetLayout_, &pipelineLayout_) ||
        !createGraphicsPipeline(vkDev, renderPass_, pipelineLayout_, {
                (FilesystemUtilities::GetShadersDir() + "Vulkan/FullScreenQuadRenderer/VK_SingleQuad.vert").c_str(),
                (FilesystemUtilities::GetShadersDir() + "Vulkan/FullScreenQuadRenderer/VK_SingleQuad.frag").c_str()
            }, &graphicsPipeline_))
    {
        printf("Failed to create pipeline\n");
        fflush(stdout);
        exit(0);
    }

    createColorAndDepthFramebuffers(vkDev, renderPass_, VK_NULL_HANDLE, swapchainFramebuffers_);
}

VulkanSingleQuadRenderer::~VulkanSingleQuadRenderer()
{
    
}