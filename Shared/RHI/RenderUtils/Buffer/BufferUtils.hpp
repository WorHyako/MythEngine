#pragma once

#include <RHI/RHICommon/RHICommon.hpp>

namespace RenderUtils
{
	/* Allocate and upload vertex & index buffer pair */
	RHI::IBuffer* addVertexBuffer(RHI::IDevice* device, RHI::IRHICommandList* commandList, uint32_t indexBufferSize, const void* indexData, uint32_t vertexBufferSize, const void* vertexData);

	RHI::IBuffer* allocateVertexBuffer(RHI::IDevice* device, RHI::IRHICommandList* commandList, size_t vertexDataSize, const void* vertexData, size_t indexDataSize, const void* indexData);

	std::pair<BufferAttachment, BufferAttachment> makeMeshBuffers(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);

	std::pair<BufferAttachment, BufferAttachment> loadMeshToBuffer(const char* filename, bool useTextureCoordinates, bool useNormals,
		std::vector<float>& vertices,
		std::vector<unsigned int>& indices);

	std::pair<BufferAttachment, BufferAttachment> createPlaneBuffer_XZ(float sx, float sz);
	std::pair<BufferAttachment, BufferAttachment> createPlaneBuffer_XY(float sx, float sy);
}
