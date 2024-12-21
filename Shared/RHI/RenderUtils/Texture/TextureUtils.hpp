#pragma once

#include <RHI/RHICommon/RHICommon.hpp>

namespace RenderUtils
{
	RHI::ITexture* createTextureImage(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* filename);
	RHI::ITexture* createTextureImageFromData(RHI::IDevice* device, RHI::IRHICommandList* commandList, RHI::TextureDesc& desc, void* imageData);
	RHI::ITexture* createMIPTextureImage(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* filename, uint32_t mipLevels);
	RHI::ITexture* createMIPTextureImageFromData(RHI::IDevice* device, RHI::IRHICommandList* commandList, RHI::TextureDesc& desc, void* mipData);

	/* Resource Management*/
	RHI::ITexture* addColorTexture(
		RHI::IDevice* device, RHI::IRHICommandList* commandList,
		int texWidth = 0, int texHeight = 0, RHI::Format colorFormat = RHI::Format::BGRA8_UNORM,
		const RHI::SamplerDesc& samplerDesc = {});
	RHI::ITexture* createDepthTexture(
		RHI::IDevice* device, RHI::IRHICommandList* commandList,
		int texWidth = 0, int texHeight = 0,
		RHI::ImageLayout layout = RHI::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	RHI::ITexture* addRGBATexture(RHI::IDevice* device, RHI::IRHICommandList* commandList, RHI::TextureDesc& desc, void* data);
	RHI::ITexture* addSolidRGBATexture(RHI::IDevice* device, RHI::IRHICommandList* commandList, uint32_t color);
	RHI::ITexture* createOffscreenImage(RHI::IDevice* device, RHI::IRHICommandList* commandList, RHI::TextureDesc& desc);
	RHI::ITexture* loadTexture2D(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* fileName);
	RHI::ITexture* createCubeTextureImage(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* filename, uint32_t* width = nullptr, uint32_t* height = nullptr);
	RHI::ITexture* createMIPCubeTextureImage(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* filename, uint32_t mipLevels, uint32_t* width = nullptr, uint32_t* height = nullptr);
	RHI::ITexture* loadCubemap(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* fileName, uint32_t mipLevels);
	RHI::ITexture* loadKTX(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* fileName);
	RHI::ITexture* createFontTexture(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* fontFile);
}
