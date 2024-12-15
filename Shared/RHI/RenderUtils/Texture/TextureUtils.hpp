#pragma once

#include <RHI/RHICommon/RHICommon.hpp>

namespace RenderUtils
{
	RHI::ITexture* createTextureImage(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* filename);
	RHI::ITexture* createTextureImageFromData(RHI::IDevice* device, RHI::IRHICommandList* commandList, RHI::TextureDesc& desc, void* imageData);
	RHI::ITexture* createMIPTextureImage(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* filename, uint32_t mipLevels);
	RHI::ITexture* createMIPTextureImageFromData(RHI::IDevice* device, RHI::IRHICommandList* commandList, RHI::TextureDesc& desc, void* mipData);

	RHI::ITexture* addColorTexture(RHI::IDevice* device, RHI::IRHICommandList* commandList, int texWidth, int texHeight, RHI::Format colorFormat, const RHI::SamplerDesc& samplerDesc);
	RHI::ITexture* addRGBATexture(RHI::IDevice* device, RHI::IRHICommandList* commandList, RHI::TextureDesc& desc, void* data);
	RHI::ITexture* addSolidRGBATexture(RHI::IDevice* device, RHI::IRHICommandList* commandList, uint32_t color);
	RHI::ITexture* loadTexture2D(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* fileName);
	RHI::ITexture* createCubeTextureImage(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* filename, uint32_t* width, uint32_t* height);
	RHI::ITexture* createMIPCubeTextureImage(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* filename, uint32_t mipLevels, uint32_t* width, uint32_t* height);
	RHI::ITexture* loadCubemap(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* fileName, uint32_t mipLevels);
	RHI::ITexture* loadKTX(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* fileName);
	RHI::ITexture* createFontTexture(RHI::IDevice* device, RHI::IRHICommandList* commandList, const char* fontFile);
}
