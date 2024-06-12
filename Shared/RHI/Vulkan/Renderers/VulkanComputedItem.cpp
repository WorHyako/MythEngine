#include <RHI/Vulkan/Renderers/VulkanComputedItem.hpp>

ComputedItem::ComputedItem(VulkanRenderDevice& vkDev, uint32_t uniformBufferSize)
    : vkDev(vkDev)
{
    uniformBuffer.size = uniformBufferSize;

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = nullptr;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateFence(vkDev.device, &fenceCreateInfo, nullptr, &fence) != VK_SUCCESS)
        exit(EXIT_FAILURE);

    if (!createUniformBuffer(vkDev, uniformBuffer.buffer, uniformBuffer.memory, uniformBuffer.size))
        exit(EXIT_FAILURE);
}

ComputedItem::~ComputedItem()
{
    vkDestroyBuffer(vkDev.device, uniformBuffer.buffer, nullptr);
    vkFreeMemory(vkDev.device, uniformBuffer.memory, nullptr);

    vkDestroyFence(vkDev.device, fence, nullptr);

    vkDestroyDescriptorPool(vkDev.device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(vkDev.device, dsLayout, nullptr);
    vkDestroyPipeline(vkDev.device, pipeline, nullptr);
    vkDestroyPipelineLayout(vkDev.device, pipelineLayout, nullptr);
}

void ComputedItem::fillComputeCommandBuffer(void* pushConstant, uint32_t pushConstantSize, uint32_t xsize, uint32_t ysize, uint32_t zsize)
{
    VkCommandBuffer commandBuffer = vkDev.computeCommandBuffer;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, 0);

    if (pushConstant && pushConstantSize > 0)
    {
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, pushConstantSize, pushConstant);
    }

    vkCmdDispatch(commandBuffer, xsize, ysize, zsize);
    vkEndCommandBuffer(commandBuffer);
}

bool ComputedItem::submit()
{
    // Use a fence to ensure that compute command buffer has finished executing before using it again
    waitFence();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkDev.computeCommandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;

    return (vkQueueSubmit(vkDev.computeQueue, 1, &submitInfo, fence) == VK_SUCCESS);
}

void ComputedItem::waitFence()
{
    vkWaitForFences(vkDev.device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkDev.device, 1, &fence);
}
