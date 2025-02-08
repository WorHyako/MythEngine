#pragma once

#include <RHICommon.hpp>

namespace RenderUtils
{
	RHI::TextureHandle createTextureImage(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* filename);
	RHI::TextureHandle createTextureImageFromData(RHI::IDevice* device, RHI::IRHICommandList* commandList, RHI::TextureDesc& desc, void* imageData);
	RHI::TextureHandle createMIPTextureImage(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* filename, uint32_t mipLevels);
	RHI::TextureHandle createMIPTextureImageFromData(RHI::IDevice* device, RHI::IRHICommandList* commandList, RHI::TextureDesc& desc, void* mipData);

	/* Resource Management*/
	RHI::TextureHandle addColorTexture(
		RHI::IDevice* device, RHI::IRHICommandList* commandList,
		int texWidth = 0, int texHeight = 0, RHI::Format colorFormat = RHI::Format::BGRA8_UNORM,
		const RHI::SamplerDesc& samplerDesc = {});
	RHI::TextureHandle createDepthTexture(
		RHI::IDevice* device, RHI::IRHICommandList* commandList,
		int texWidth = 0, int texHeight = 0,
		RHI::ImageLayout layout = RHI::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	RHI::TextureHandle addRGBATexture(RHI::IDevice* device, RHI::IRHICommandList* commandList, RHI::TextureDesc& desc, void* data);
	RHI::TextureHandle addSolidRGBATexture(RHI::IDevice* device, RHI::IRHICommandList* commandList, uint32_t color);
	RHI::TextureHandle createOffscreenImage(RHI::IDevice* device, RHI::IRHICommandList* commandList, RHI::TextureDesc& desc);
	RHI::TextureHandle loadTexture2D(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* fileName);
	RHI::TextureHandle createCubeTextureImage(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* filename, uint32_t* width = nullptr, uint32_t* height = nullptr);
	RHI::TextureHandle createMIPCubeTextureImage(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* filename, uint32_t mipLevels, uint32_t* width = nullptr, uint32_t* height = nullptr);
	RHI::TextureHandle loadCubemap(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* fileName, uint32_t mipLevels);
	RHI::TextureHandle loadBRDFLUTKTX(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* fileName);
	RHI::TextureHandle loadKTX(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* fileName);
	RHI::TextureHandle createFontTexture(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* fontFile);
}
