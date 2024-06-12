#include <RHI/Vulkan/Framework/FinalRenderer.hpp>

#include <stb_image.h>

#include <Filesystem/FilesystemUtilities.hpp>

BaseMultiRenderer::BaseMultiRenderer(
	VulkanRenderContext& ctx,
	VKSceneData& sceneData,
	const std::vector<int>& objectIndices,
	const char* vertShaderFile,
	const char* fragShaderFile,
	const std::vector<VulkanTexture>& outputs,
	RenderPass screenRenderPass,
	const std::vector<BufferAttachment>& auxBuffers,
	const std::vector<TextureAttachment>& auxTextures)
	: Renderer(ctx)
	, sceneData_(sceneData)
	, indices_(objectIndices)
{
	const PipelineInfo pInfo = initRenderPass(PipelineInfo{}, outputs, screenRenderPass, ctx.screenRenderPass);

	const uint32_t indirectDataSize = (uint32_t)sceneData_.shapes_.size() * sizeof(VkDrawIndirectCommand);

	const size_t imgCount = ctx.vkDev.swapchainImages.size();
	uniforms_.resize(imgCount);
	shape_.resize(imgCount);
	indirect_.resize(imgCount);

	descriptorSets_.resize(imgCount);

	const uint32_t shapesSize = (uint32_t)sceneData_.shapes_.size() * sizeof(DrawData);
	const uint32_t uniformBufferSize = sizeof(ubo_);

	std::vector<TextureAttachment> textureAttachments;
	if (sceneData_.envMap_.width)
		textureAttachments.push_back(fsTextureAttachment(sceneData_.envMap_));
	if (sceneData_.envMapIrradiance_.width)
		textureAttachments.push_back(fsTextureAttachment(sceneData_.envMapIrradiance_));
	if (sceneData_.brdfLUT_.width)
		textureAttachments.push_back(fsTextureAttachment(sceneData_.brdfLUT_));

	for (const auto& t : auxTextures)
		textureAttachments.push_back(t);

	DescriptorSetInfo dsInfo = {
		{
			uniformBufferAttachment(VulkanBuffer {},         0, uniformBufferSize, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
			sceneData_.vertexBuffer_,
			sceneData_.indexBuffer_,
			storageBufferAttachment(VulkanBuffer {},         0, shapesSize, VK_SHADER_STAGE_VERTEX_BIT),
			storageBufferAttachment(sceneData_.material_,    0, (uint32_t)sceneData_.material_.size, VK_SHADER_STAGE_FRAGMENT_BIT),
			storageBufferAttachment(sceneData_.transforms_,  0, (uint32_t)sceneData_.transforms_.size, VK_SHADER_STAGE_VERTEX_BIT),
		},
		textureAttachments,
		{ sceneData_.allMaterialTextures }
	};

	for (const auto& b : auxBuffers)
		dsInfo.buffers.push_back(b);

	descriptorSetLayout_ = ctx.resources.addDescriptorSetLayout(dsInfo);
	descriptorPool_ = ctx.resources.addDescriptorPool(dsInfo, (uint32_t)imgCount);

	for (size_t i = 0; i != imgCount; i++)
	{
		uniforms_[i] = ctx.resources.addUniformBuffer(uniformBufferSize);
		/*indirect_[i] = ctx.resources.addIndirectBuffer(indirectDataSize);
		updateIndirectBuffers(i);*/



		indirect_[i].buffer = VK_NULL_HANDLE;
		indirect_[i].size = 0;
		indirect_[i].memory = VK_NULL_HANDLE;
		indirect_[i].ptr = nullptr;
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(ctx_.vkDev.device, ctx_.vkDev.physicalDevice, indirectDataSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		updateIndirectBuffersOptimized(&stagingBufferMemory);

		createBuffer(ctx_.vkDev.device, ctx_.vkDev.physicalDevice, indirectDataSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indirect_[i].buffer, indirect_[i].memory);

		copyBuffer(ctx_.vkDev, stagingBuffer, indirect_[i].buffer, indirectDataSize);

		vkDestroyBuffer(ctx_.vkDev.device, stagingBuffer, nullptr);
		vkFreeMemory(ctx_.vkDev.device, stagingBufferMemory, nullptr);



		shape_[i] = ctx.resources.addStorageBuffer(shapesSize);
		uploadBufferData(ctx.vkDev, shape_[i].memory, 0, sceneData_.shapes_.data(), shapesSize);

		dsInfo.buffers[0].buffer = uniforms_[i];
		dsInfo.buffers[3].buffer = shape_[i];

		descriptorSets_[i] = ctx.resources.addDescriptorSet(descriptorPool_, descriptorSetLayout_);
		ctx.resources.updateDescriptorSet(descriptorSets_[i], dsInfo);
	}

	initPipeline({ vertShaderFile, fragShaderFile }, pInfo);
}


void BaseMultiRenderer::fillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage, VkFramebuffer fb, VkRenderPass rp)
{
	beginRenderPass((rp != VK_NULL_HANDLE) ? rp : renderPass_.handle, (fb != VK_NULL_HANDLE) ? fb : framebuffer_, commandBuffer, currentImage);

	/* For CountKHR (Vulkan 1.1) we may use indirect rendering with GPU-based object counter */
	/// vkCmdDrawIndirectCountKHR(commandBuffer, indirectBuffers_[currentImage], 0, countBuffers_[currentImage], 0, shapes.size(), sizeof(VkDrawIndirectCommand));
	/* For Vulkan 1.0 vkCmdDrawIndirect is enough */
	vkCmdDrawIndirect(commandBuffer, indirect_[currentImage].buffer, 0, (uint32_t)sceneData_.shapes_.size(), sizeof(VkDrawIndirectCommand));

	vkCmdEndRenderPass(commandBuffer);
}

void BaseMultiRenderer::updateIndirectBuffers(size_t currentImage, bool* visibility)
{
	VkDrawIndirectCommand* data = nullptr;
	vkMapMemory(ctx_.vkDev.device, indirect_[currentImage].memory, 0, sizeof(VkDrawIndirectCommand), 0, (void**)&data);

	const uint32_t size = (uint32_t)indices_.size(); // (uint32_t)sceneData_.shapes_.size();

	for (uint32_t i = 0; i != size; i++)
	{
		const uint32_t j = sceneData_.shapes_[indices_[i]].meshIndex;

		const uint32_t lod = sceneData_.shapes_[indices_[i]].LOD;
		data[i].vertexCount = sceneData_.meshData_.meshes_[j].getLODIndicesCount(lod);
		data[i].instanceCount = visibility ? (visibility[indices_[i]] ? 1u : 0u) : 1u;
		data[i].firstVertex = 0;
		data[i].firstInstance = (uint32_t)indices_[i];
	}
	vkUnmapMemory(ctx_.vkDev.device, indirect_[currentImage].memory);
}

void BaseMultiRenderer::updateIndirectBuffersOptimized(VkDeviceMemory* indirectTransferMemory, bool* visibility)
{
	VkDrawIndirectCommand* data = nullptr;
	vkMapMemory(ctx_.vkDev.device, *indirectTransferMemory, 0, sizeof(VkDrawIndirectCommand), 0, (void**)&data);

	const uint32_t size = (uint32_t)indices_.size(); // (uint32_t)sceneData_.shapes_.size();

	for (uint32_t i = 0; i != size; i++)
	{
		const uint32_t j = sceneData_.shapes_[indices_[i]].meshIndex;

		const uint32_t lod = sceneData_.shapes_[indices_[i]].LOD;
		data[i].vertexCount = sceneData_.meshData_.meshes_[j].getLODIndicesCount(lod);
		data[i].instanceCount = visibility ? (visibility[indices_[i]] ? 1u : 0u) : 1u;
		data[i].firstVertex = 0;
		data[i].firstInstance = (uint32_t)indices_[i];
	}
	vkUnmapMemory(ctx_.vkDev.device, *indirectTransferMemory);
}

FinalMultiRenderer::FinalMultiRenderer(
	VulkanRenderContext& ctx,
	VKSceneData& sceneData,
	const std::vector<VulkanTexture>& outputs)
	: Renderer(ctx)
	, shadowColor(ctx_.resources.addColorTexture(ShadowSize, ShadowSize))
	, shadowDepth(ctx_.resources.addDepthTexture(ShadowSize, ShadowSize))
	, lightParams(ctx_.resources.addStorageBuffer(sizeof(LightParamsBuffer)))
	, atomicBuffer(ctx_.resources.addStorageBuffer(sizeof(uint32_t)))
	, headsBuffer(ctx_.resources.addLocalDeviceStorageBuffer(ctx.vkDev.framebufferWidth* ctx.vkDev.framebufferHeight * sizeof(uint32_t)))
	, oitBuffer(ctx_.resources.addLocalDeviceStorageBuffer(ctx.vkDev.framebufferWidth* ctx.vkDev.framebufferHeight * sizeof(TransparentFragment)))
	, outputColor(ctx_.resources.addColorTexture(0, 0, LuminosityFormat))
	, sceneData_(sceneData)
	, opaqueRenderer(ctx, sceneData, getOpaqueIndices(sceneData), 
		(FilesystemUtilities::GetShadersDir() + "Vulkan/ShadowMapping/SceneIBL.vert").c_str(),
		(FilesystemUtilities::GetShadersDir() + "Vulkan/ShadowMapping/SceneIBL.frag").c_str(),
		outputs, ctx_.resources.addRenderPass(outputs, RenderPassCreateInfo{false, false, eRenderPassBit_Offscreen }),
		{ storageBufferAttachment(lightParams, 0, sizeof(LightParamsBuffer), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) },
		{ fsTextureAttachment(shadowDepth) })

	, transparentRenderer(ctx, sceneData, getTransparentIndices(sceneData),
		(FilesystemUtilities::GetShadersDir() + "Vulkan/ShadowMapping/SceneIBL.vert").c_str(),
		(FilesystemUtilities::GetShadersDir() + "Vulkan/OITransparency/GlassIBL.frag").c_str(),
		outputs, ctx_.resources.addRenderPass(outputs, RenderPassCreateInfo{false, false, eRenderPassBit_Offscreen }),
		{ storageBufferAttachment(lightParams, 0, sizeof(LightParamsBuffer), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
		  storageBufferAttachment(atomicBuffer, 0, sizeof(uint32_t), VK_SHADER_STAGE_FRAGMENT_BIT),
		  storageBufferAttachment(headsBuffer,  0, ctx.vkDev.framebufferWidth * ctx.vkDev.framebufferHeight * sizeof(uint32_t), VK_SHADER_STAGE_FRAGMENT_BIT),
		  storageBufferAttachment(oitBuffer, 0, ctx.vkDev.framebufferWidth * ctx.vkDev.framebufferHeight * sizeof(TransparentFragment), VK_SHADER_STAGE_FRAGMENT_BIT) },
		{ fsTextureAttachment(shadowDepth) })

	, shadowRenderer(ctx_, sceneData, getOpaqueIndices(sceneData),
		(FilesystemUtilities::GetShadersDir() + "Vulkan/ShadowMapping/IndirectShadowMapping.vert").c_str(),
		(FilesystemUtilities::GetShadersDir() + "Vulkan/ShadowMapping/IndirectShadowMapping.frag").c_str(),
		{ shadowColor, shadowDepth },
		ctx_.resources.addRenderPass({ shadowColor, shadowDepth },
			RenderPassCreateInfo{true, true, eRenderPassBit_First | eRenderPassBit_Offscreen }))

	, colorToAttachment(ctx_, outputs[0])
	, depthToAttachment(ctx_, outputs[1])

	, whBuffer(ctx_.resources.addUniformBuffer(sizeof(UBO)))

	, clearOIT(ctx_, {{
			uniformBufferAttachment(whBuffer,         0, sizeof(ubo_), VK_SHADER_STAGE_FRAGMENT_BIT),
			storageBufferAttachment(headsBuffer,  0, ctx.vkDev.framebufferWidth * ctx.vkDev.framebufferHeight * sizeof(uint32_t), VK_SHADER_STAGE_FRAGMENT_BIT)
		 }, { fsTextureAttachment(outputs[0]) } },
		{ outputColor }, (FilesystemUtilities::GetShadersDir() + "Vulkan/OITransparency/ClearBuffer.frag").c_str())

	, composeOIT(ctx_,
		{
			{
				uniformBufferAttachment(whBuffer,     0, sizeof(ubo_),     VK_SHADER_STAGE_FRAGMENT_BIT),
				storageBufferAttachment(headsBuffer,  0, ctx.vkDev.framebufferWidth * ctx.vkDev.framebufferHeight * sizeof(uint32_t), VK_SHADER_STAGE_FRAGMENT_BIT),
				storageBufferAttachment(oitBuffer,    0, ctx.vkDev.framebufferWidth * ctx.vkDev.framebufferHeight * sizeof(TransparentFragment), VK_SHADER_STAGE_FRAGMENT_BIT)
			},
		{ fsTextureAttachment(outputs[0]) }
		}, { outputColor }, (FilesystemUtilities::GetShadersDir() + "Vulkan/OITransparency/ComposeOIT.frag").c_str())

	, outputToAttachment(ctx_, outputColor)
	, outputToShader(ctx_, outputColor)
{
	ubo_.width = ctx.vkDev.framebufferWidth;
	ubo_.height = ctx.vkDev.framebufferHeight;

	setVkImageName(ctx_.vkDev, outputColor.image.image, "outputColor");
}

void FinalMultiRenderer::fillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage, VkFramebuffer fb, VkRenderPass rp)
{
	outputToAttachment.fillCommandBuffer(commandBuffer, currentImage);

	clearOIT.fillCommandBuffer(commandBuffer, currentImage);

	if (enableShadows)
	{
		shadowRenderer.fillCommandBuffer(commandBuffer, currentImage);
	}

	opaqueRenderer.fillCommandBuffer(commandBuffer, currentImage);

	VkBufferMemoryBarrier headsBufferBarrier{};
	headsBufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	headsBufferBarrier.pNext = nullptr;
	headsBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	headsBufferBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
	headsBufferBarrier.srcQueueFamilyIndex = 0;
	headsBufferBarrier.dstQueueFamilyIndex = 0;
	headsBufferBarrier.buffer = headsBuffer.buffer;
	headsBufferBarrier.offset = 0;
	headsBufferBarrier.size = headsBuffer.size;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &headsBufferBarrier, 0, nullptr);

	if (renderTransparentObjects)
	{
		colorToAttachment.fillCommandBuffer(commandBuffer, currentImage);
		depthToAttachment.fillCommandBuffer(commandBuffer, currentImage);

		transparentRenderer.fillCommandBuffer(commandBuffer, currentImage);

		VkMemoryBarrier readoutBarrier2{};
		readoutBarrier2.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		readoutBarrier2.pNext = nullptr;
		readoutBarrier2.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		readoutBarrier2.dstAccessMask = VK_ACCESS_HOST_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &readoutBarrier2, 0, nullptr, 0, nullptr);
	}

	composeOIT.fillCommandBuffer(commandBuffer, currentImage);
	outputToShader.fillCommandBuffer(commandBuffer, currentImage);
}

void FinalMultiRenderer::updateBuffers(size_t currentImage)
{
	transparentRenderer.updateBuffers(currentImage);
	opaqueRenderer.updateBuffers(currentImage);

	shadowRenderer.updateBuffers(currentImage);

	uint32_t zeroCount = 0;
	uploadBufferData(ctx_.vkDev, atomicBuffer.memory, 0, &zeroCount, sizeof(uint32_t));

	uploadBufferData(ctx_.vkDev, whBuffer.memory, 0, &ubo_, sizeof(ubo_));
}

bool FinalMultiRenderer::checkLoadedTextures()
{
	VKSceneData::LoadedImageData data;

	{
		std::lock_guard lock(sceneData_.loadedFilesMutex_);

		if (sceneData_.loadedFiles_.empty())
			return false;

		data = sceneData_.loadedFiles_.back();

		sceneData_.loadedFiles_.pop_back();
	}

	auto newTexture = ctx_.resources.addRGBATexture(data.w_, data.h_, const_cast<uint8_t*>(data.img_));

	transparentRenderer.updateTexture(data.index_, newTexture, 14);
	opaqueRenderer.updateTexture(data.index_, newTexture, 11);

	stbi_image_free((void*)data.img_);

	return true;
}
