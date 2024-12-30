#pragma once

#include <RHICommon.hpp>

namespace RenderUtils
{
	/* Allocate and upload vertex & index buffer pair */
	RHI::IBuffer* addVertexBuffer(RHI::IDevice* device, RHI::IRHICommandList* commandList, uint32_t indexBufferSize, const void* indexData, uint32_t vertexBufferSize, const void* vertexData);

	RHI::IBuffer* allocateVertexBuffer(RHI::IDevice* device, RHI::IRHICommandList* commandList, size_t vertexDataSize, const void* vertexData, size_t indexDataSize, const void* indexData);

	std::pair<RHI::BufferAttachment, RHI::BufferAttachment> makeMeshBuffers(RHI::IDevice* device, RHI::IRHICommandList* commandList, const std::vector<float>& vertices, const std::vector<unsigned int>& indices);

	std::pair<RHI::BufferAttachment, RHI::BufferAttachment> loadMeshToBuffer(
		RHI::IDevice* device, RHI::IRHICommandList* commandList, 
		const char* filename, bool useTextureCoordinates, bool useNormals,
		std::vector<float>& vertices,
		std::vector<unsigned int>& indices);

	std::pair<RHI::BufferAttachment, RHI::BufferAttachment> createPlaneBuffer_XZ(RHI::IDevice* device, RHI::IRHICommandList* commandList, float sx, float sz);
	std::pair<RHI::BufferAttachment, RHI::BufferAttachment> createPlaneBuffer_XY(RHI::IDevice* device, RHI::IRHICommandList* commandList, float sx, float sy);
}
