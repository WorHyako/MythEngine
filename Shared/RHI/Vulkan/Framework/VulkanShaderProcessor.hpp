#pragma once

#include <RHI/Vulkan/Framework/Renderer.hpp>

#include <Filesystem/FilesystemUtilities.hpp>

/*
   @brief Shader (post)processor for fullscreen effects

   Multiple input textures, single output [color + depth]. Possibly, can be extented to multiple outputs (allocate appropriate framebuffer)
*/
struct VulkanShaderProcessor : public Renderer
{
	VulkanShaderProcessor(VulkanRenderContext& ctx,
		const PipelineInfo& pInfo,
		const DescriptorSetInfo& dsInfo,
		const std::vector<const char*>& shaders,
		const std::vector<VulkanTexture>& outputs,
		uint32_t indexBufferSize = 6 * 4,
		RenderPass screenRenderPass = RenderPass());

	void fillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE) override;

private:
	uint32_t indexBufferSize;
};

struct QuadProcessor : public VulkanShaderProcessor
{
	QuadProcessor(VulkanRenderContext& ctx, const DescriptorSetInfo& dsInfo,
		const std::vector<VulkanTexture>& outputs, const char* shaderFile)
			: VulkanShaderProcessor(ctx, ctx.pipelineParametersForOutputs(outputs), dsInfo,
				std::vector<const char*>{(FilesystemUtilities::GetShadersDir() + "Vulkan/FullScreenQuadRenderer/VK_SingleQuad.vert").c_str(), shaderFile},
				outputs, 6 * 4, outputs.empty() ? ctx.screenRenderPass : RenderPass())
	{}
};

struct BufferProcessor : public VulkanShaderProcessor
{
	BufferProcessor(VulkanRenderContext& ctx, const DescriptorSetInfo& dsInfo,
		const std::vector<VulkanTexture>& outputs, const std::vector<const char*>& shaderFiles,
		uint32_t indexBufferSize = 6 * 4, RenderPass renderPass = RenderPass())
		: VulkanShaderProcessor(ctx, ctx.pipelineParametersForOutputs(outputs), dsInfo,
			shaderFiles, outputs, indexBufferSize, outputs.empty() ? ctx.screenRenderPass : renderPass)
	{}
};

/* Commonly used BufferProcessor for single mesh rendering */
struct OffscreenMeshRenderer : public BufferProcessor
{
	OffscreenMeshRenderer(
		VulkanRenderContext& ctx,
		VulkanBuffer uniformBuffer,
		const std::pair<BufferAttachment, BufferAttachment>& meshBuffer,
		const std::vector<TextureAttachment>& usedTextures,
		const std::vector<VulkanTexture>& outputs,
		const std::vector<const char*>& shaderFiles,
		bool firstPass = false)
			:
		BufferProcessor(ctx,
			DescriptorSetInfo{
				{
					uniformBufferAttachment(uniformBuffer, 0, 0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
					meshBuffer.first,
					meshBuffer.second
				},
				usedTextures
			},
			outputs, shaderFiles, meshBuffer.first.size,
			ctx.resources.addRenderPass(outputs, RenderPassCreateInfo{
				firstPass, firstPass, (uint8_t)((firstPass ? eRenderPassBit_First : eRenderPassBit_OffscreenInternal) | eRenderPassBit_Offscreen)
			}))
	{}
};