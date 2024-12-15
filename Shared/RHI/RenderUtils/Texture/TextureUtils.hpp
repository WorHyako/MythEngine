#pragma once

#include <RHI/RHICommon/RHICommon.hpp>

namespace RenderUtils
{
	using namespace RHI;

	ITexture* createTextureImage(IDevice* device, IRHICommandList* commandList, const char* filename);
	ITexture* createTextureImageFromData(IDevice* device, IRHICommandList* commandList, TextureDesc& desc, void* imageData);
	ITexture* createMIPTextureImage(IDevice* device, IRHICommandList* commandList, const char* filename, uint32_t mipLevels);
	ITexture* createMIPTextureImageFromData(IDevice* device, IRHICommandList* commandList, void* mipData, TextureDesc& desc);

	ITexture* addRGBATexture(IDevice* device, IRHICommandList* commandList, TextureDesc& desc, void* data);
	ITexture* addSolidRGBATexture(IDevice* device, IRHICommandList* commandList, uint32_t color);
	ITexture* loadKTX(IDevice* device, IRHICommandList* commandList, const char* fileName);
	ITexture* loadTexture2D(IDevice* device, IRHICommandList* commandList, const char* fileName);
	ITexture* loadCubemap(IDevice* device, IRHICommandList* commandList, const char* fileName, uint32_t mipLevels);
	ITexture* createFontTexture(IDevice* device, IRHICommandList* commandList, const char* fontFile);
}
