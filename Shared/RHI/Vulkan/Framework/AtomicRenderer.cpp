#include <RHI/Vulkan/Framework/AtomicRenderer.hpp>

#include <RHI/Vulkan/Framework/VulkanApp.hpp>

#include <Filesystem/FilesystemUtilities.hpp>

struct node
{
	uint32_t idx;
	float xx, yy;
};

AtomicRenderer::AtomicRenderer(VulkanRenderContext& ctx, VulkanBuffer sizeBuffer)
	: Renderer(ctx)
{
	const PipelineInfo pInfo = initRenderPass(PipelineInfo{}, { ctx.resources.addColorTexture() }, RenderPass(), ctx.screenRenderPass_NoDepth);

	uint32_t W = ctx.vkDev.framebufferWidth;
	uint32_t H = ctx.vkDev.framebufferHeight;

	const size_t imgCount = ctx.vkDev.swapchainImages.size();
	descriptorSets_.resize(imgCount);
	atomics_.resize(imgCount);
	output_.resize(imgCount);

	DescriptorSetInfo dsInfo = {
			{
				storageBufferAttachment(VulkanBuffer {}, 0,     sizeof(uint32_t), VK_SHADER_STAGE_FRAGMENT_BIT),
				storageBufferAttachment(VulkanBuffer {}, 0, W * H * sizeof(node), VK_SHADER_STAGE_FRAGMENT_BIT),
				uniformBufferAttachment(sizeBuffer,      0,                    8, VK_SHADER_STAGE_FRAGMENT_BIT)
			}
	};

	descriptorSetLayout_ = ctx.resources.addDescriptorSetLayout(dsInfo);
	descriptorPool_ = ctx.resources.addDescriptorPool(dsInfo, (uint32_t)imgCount);

	for (size_t i = 0; i < imgCount; i++)
	{
		atomics_[i] = ctx.resources.addStorageBuffer(sizeof(uint32_t));
		output_[i] = ctx.resources.addStorageBuffer(W * H * sizeof(node));
		dsInfo.buffers[0].buffer = atomics_[i];
		dsInfo.buffers[1].buffer = output_[i];

		descriptorSets_[i] = ctx.resources.addDescriptorSet(descriptorPool_, descriptorSetLayout_);
		ctx.resources.updateDescriptorSet(descriptorSets_[i], dsInfo);
	}

	initPipeline(
		{ (FilesystemUtilities::GetShadersDir() + "Vulkan/AtomicTest/AtomicTest.vert").c_str(),
			(FilesystemUtilities::GetShadersDir() + "Vulkan/AtomicTest/AtomicTest.frag").c_str() }, pInfo);
}

void AtomicRenderer::fillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage, VkFramebuffer fb, VkRenderPass rp)
{
	beginRenderPass((rp != VK_NULL_HANDLE) ? rp : renderPass_.handle, (fb != VK_NULL_HANDLE) ? fb : framebuffer_, commandBuffer, currentImage);
	vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);
}

void AtomicRenderer::updateBuffers(size_t currentImage)
{
	uint32_t zeroCount = 0;
	uploadBufferData(ctx_.vkDev, atomics_[currentImage].memory, 0, &zeroCount, sizeof(uint32_t));
}

AnimRenderer::AnimRenderer(VulkanRenderContext& ctx, std::vector<VulkanBuffer>& pointBuffers, VulkanBuffer sizeBuffer)
	: Renderer(ctx)
	, pointBuffers_(pointBuffers)
{
	initRenderPass(PipelineInfo{}, {}, RenderPass(), ctx.screenRenderPass_NoDepth);

	const size_t imgCount = ctx.vkDev.swapchainImages.size();
	descriptorSets_.resize(imgCount);

	uint32_t W = ctx.vkDev.framebufferWidth;
	uint32_t H = ctx.vkDev.framebufferHeight;

	DescriptorSetInfo dsInfo = {
			{
				storageBufferAttachment(VulkanBuffer {}, 0, W * H * sizeof(node), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
				uniformBufferAttachment(sizeBuffer,      0,                    8, VK_SHADER_STAGE_VERTEX_BIT)
			}
	};

	descriptorSetLayout_ = ctx.resources.addDescriptorSetLayout(dsInfo);
	descriptorPool_ = ctx.resources.addDescriptorPool(dsInfo, (uint32_t)imgCount);

	for (size_t i = 0; i < imgCount; i++)
	{
		dsInfo.buffers[0].buffer = pointBuffers_[i];
		descriptorSets_[i] = ctx.resources.addDescriptorSet(descriptorPool_, descriptorSetLayout_);
		ctx.resources.updateDescriptorSet(descriptorSets_[i], dsInfo);
	}

	PipelineInfo pInfo{};
	pInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	initPipeline({ (FilesystemUtilities::GetShadersDir() + "Vulkan/AtomicTest/AtomicVisualize.vert").c_str(),
			(FilesystemUtilities::GetShadersDir() + "Vulkan/AtomicTest/AtomicVisualize.frag").c_str() }, pInfo);
}

void AnimRenderer::fillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage, VkFramebuffer fb, VkRenderPass rp)
{
	uint32_t pointCount =
		uint32_t(ctx_.vkDev.framebufferWidth * ctx_.vkDev.framebufferHeight * percentage);

	if (pointCount == 0)
		return;

	beginRenderPass((rp != VK_NULL_HANDLE) ? rp : renderPass_.handle, (fb != VK_NULL_HANDLE) ? fb : framebuffer_, commandBuffer, currentImage);
	vkCmdDraw(commandBuffer, pointCount, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);
}
