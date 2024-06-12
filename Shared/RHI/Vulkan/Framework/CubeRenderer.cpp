#include <RHI/Vulkan/Framework/CubeRenderer.hpp>

#include <Filesystem/FilesystemUtilities.hpp>

CubemapRenderer::CubemapRenderer(VulkanRenderContext& ctx,
                                 VulkanTexture texture,
                                 const std::vector<VulkanTexture>& outputs,
                                 RenderPass screenRenderPass)
	: Renderer(ctx)
{
	const PipelineInfo pInfo = initRenderPass(PipelineInfo{}, outputs, screenRenderPass, ctx.screenRenderPass);

	const size_t imgCount = ctx.vkDev.swapchainImages.size();
	descriptorSets_.resize(imgCount);
	uniforms_.resize(imgCount);

	DescriptorSetInfo dsInfo{};
	dsInfo.buffers = { uniformBufferAttachment(VulkanBuffer{}, 0, sizeof(UniformBuffer), VK_SHADER_STAGE_VERTEX_BIT) };
	dsInfo.textures = { fsTextureAttachment(texture) };

	descriptorSetLayout_ = ctx.resources.addDescriptorSetLayout(dsInfo);
	descriptorPool_ = ctx.resources.addDescriptorPool(dsInfo, imgCount);

	for(size_t i = 0; i != imgCount; i++)
	{
		uniforms_[i] = ctx.resources.addUniformBuffer(sizeof(UniformBuffer));
		dsInfo.buffers[0].buffer = uniforms_[i];

		descriptorSets_[i] = ctx.resources.addDescriptorSet(descriptorPool_, descriptorSetLayout_);
		ctx.resources.updateDescriptorSet(descriptorSets_[i], dsInfo);
	}

	initPipeline({ (FilesystemUtilities::GetShadersDir() + "Vulkan/HDR/Cubemap.vert").c_str(), (FilesystemUtilities::GetShadersDir() + "Vulkan/HDR/Cubemap.frag").c_str() }, pInfo);
}

void CubemapRenderer::fillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage, VkFramebuffer fb, VkRenderPass rp)
{
	beginRenderPass((rp != VK_NULL_HANDLE) ? rp : renderPass_.handle, (fb != VK_NULL_HANDLE) ? fb : framebuffer_, commandBuffer, currentImage);

	vkCmdDraw(commandBuffer, 36, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);
}

void CubemapRenderer::updateBuffers(size_t currentImage)
{
	updateUniformBuffer(currentImage, 0, sizeof(UniformBuffer), &ubo);
}
