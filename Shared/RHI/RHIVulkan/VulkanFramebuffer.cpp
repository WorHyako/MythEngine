#include "RHI/RHIVulkan/VulkanBackend.hpp"
#include <array>

namespace RHI::Vulkan
{
	bool VulkanDevice::createColorAndDepthFramebuffers(VkRenderPass renderPass, VkImageView depthImageView, std::vector<VkFramebuffer>& swapchainFramebuffers)
    {
        swapchainFramebuffers.resize( ctx_.vkDev.swapchainImageViews.size());

        for (size_t i = 0; i < ctx_.vkDev.swapchainImages.size(); i++)
        {
            std::array<VkImageView, 2> attachments = {
                    ctx_.vkDev.swapchainImageViews[i],
                    depthImageView
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.pNext = nullptr;
            framebufferInfo.flags = 0;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>((depthImageView == VK_NULL_HANDLE) ? 1 : 2);
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = ctx_.vkDev.framebufferWidth;
            framebufferInfo.height = ctx_.vkDev.framebufferHeight;
            framebufferInfo.layers = 1;

            VK_CHECK(vkCreateFramebuffer(ctx_.vkDev.device, &framebufferInfo, nullptr, &swapchainFramebuffers[i]));
        }

        return true;
    }

    VkFramebuffer VulkanDevice::addFramebuffer(RenderPass renderPass, const std::vector<VulkanTexture>& images)
    {
        VkFramebuffer framebuffer;

        std::vector<VkImageView> attachments;
        for (const auto& i : images)
            attachments.push_back(i.image.imageView);

        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.pNext = nullptr;
        fbInfo.flags = 0;
        fbInfo.renderPass = renderPass.handle;
        fbInfo.attachmentCount = (uint32_t)attachments.size();
        fbInfo.pAttachments = attachments.data();
        fbInfo.width = images[0].width;
        fbInfo.height = images[0].height;
        fbInfo.layers = 1;

        if (vkCreateFramebuffer(ctx_.vkDev.device, &fbInfo, nullptr, &framebuffer) != VK_SUCCESS)
        {
            printf("Unable to create offscreen framebuffer\n");
            exit(EXIT_FAILURE);
        }

        resources_.allFramebuffers.push_back(framebuffer);
        return framebuffer;
    }

    std::vector<VkFramebuffer> VulkanDevice::addFramebuffers(VkRenderPass renderPass, VkImageView depthView)
    {
        std::vector<VkFramebuffer> framebuffers;
        createColorAndDepthFramebuffers(renderPass, depthView, framebuffers);
        for (auto f : framebuffers)
            resources_.allFramebuffers.push_back(f);
        return framebuffers;
    }
}
