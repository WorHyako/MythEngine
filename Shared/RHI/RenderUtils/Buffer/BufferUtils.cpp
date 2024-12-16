#include <RHI/RenderUtils/Buffer/BufferUtils.hpp>

#include "assimp/cimport.h"
#include "assimp/mesh.h"

namespace RenderUtils
{
	using namespace RHI;

    IBuffer* addVertexBuffer(IDevice* device, IRHICommandList* commandList, uint32_t indexBufferSize, const void* indexData, uint32_t vertexBufferSize, const void* vertexData)
    {
        return allocateVertexBuffer(device, commandList, vertexBufferSize, vertexData, indexBufferSize, indexData);
        //m_Resources.allBuffers.push_back(result);
    }

    IBuffer* allocateVertexBuffer(RHI::IDevice* device, RHI::IRHICommandList* commandList, size_t vertexDataSize, const void* vertexData, size_t indexDataSize, const void* indexData)
    {
        size_t bufferSize = vertexDataSize + indexDataSize;

        BufferDesc stagingDesc = BufferDesc{}
            .setSize(bufferSize)
            .setIsTransferSrc(true)
            .setMemoryProperties(MemoryPropertiesBits::HOST_VISIBLE_BIT | MemoryPropertiesBits::HOST_COHERENT_BIT);
        IBuffer* stagingBuffer = device->createBuffer(stagingDesc);

        void* data;
        vkMapMemory(m_Context.device, stagingBuffer->memory, 0, bufferSize, 0, &data);
        memcpy(data, vertexData, vertexDataSize);
        memcpy((unsigned char*)data + vertexDataSize, indexData, indexDataSize);
        vkUnmapMemory(m_Context.device, stagingBuffer->memory);

        BufferDesc storageDesc = BufferDesc{}
            .setSize(bufferSize)
            .setIsTransferDst(true)
            .setIsStorageBuffer(true)
            .setMemoryProperties(MemoryPropertiesBits::DEVICE_LOCAL_BIT);
        IBuffer* storageBuffer = device->createBuffer(storageDesc);
        storageBuffer->size = bufferSize;

        commandList->copyBuffer(stagingBuffer, storageBuffer, bufferSize);

        delete stagingBuffer;

        return storageBuffer;
    }

    /* Helper mesh-related functions */
    std::pair<BufferAttachment, BufferAttachment> Device::makeMeshBuffers(const std::vector<float>& vertices, const std::vector<unsigned>& indices)
    {
        const uint32_t indexBufferSize = uint32_t(indices.size() * sizeof(int));
        const uint32_t vertexBufferSize = uint32_t(vertices.size() * sizeof(float));

        IBuffer* storageBuffer = addVertexBuffer(indexBufferSize, indices.data(), vertexBufferSize, vertices.data());

        BufferAttachment vertexBufferAttachment{};
        vertexBufferAttachment.dInfo = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT };
        vertexBufferAttachment.buffer = storageBuffer;
        vertexBufferAttachment.offset = 0;
        vertexBufferAttachment.size = vertexBufferSize;
        BufferAttachment indexBufferAttachment{};
        indexBufferAttachment.dInfo = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT };
        indexBufferAttachment.buffer = storageBuffer;
        indexBufferAttachment.offset = vertexBufferSize;
        indexBufferAttachment.size = indexBufferSize;

        return { vertexBufferAttachment, indexBufferAttachment };
    }

    std::pair<BufferAttachment, BufferAttachment> Device::loadMeshToBuffer(
        const char* filename,
        bool useTextureCoordinates,
        bool useNormals,
        std::vector<float>& vertices,
        std::vector<unsigned>& indices)
    {
        const aiScene* scene = aiImportFile(filename, aiProcess_Triangulate);

        if (!scene || !scene->HasMeshes())
        {
            printf("Unable to load %s\n", filename);
            //Buffer nullBuffer{ VK_NULL_HANDLE, 0, VK_NULL_HANDLE };
            Buffer* nullBuffer = nullptr;

            return std::pair{ BufferAttachment { DescriptorInfo {} , nullBuffer } , BufferAttachment { DescriptorInfo {}, nullBuffer } };
        }

        const aiMesh* mesh = scene->mMeshes[0];

        for (unsigned i = 0; i != mesh->mNumVertices; i++)
        {
            const aiVector3D v = mesh->mVertices[i];
            const aiVector3D t = mesh->mTextureCoords[0] ? mesh->mTextureCoords[0][i] : aiVector3D();
            const aiVector3D n = mesh->mNormals ? mesh->mNormals[i] : aiVector3D();

            vertices.push_back(v.x);
            vertices.push_back(v.y);
            vertices.push_back(v.z);

            if (useTextureCoordinates)
            {
                vertices.push_back(t.x);
                vertices.push_back(t.y);
            }

            if (useNormals)
            {
                vertices.push_back(n.x);
                vertices.push_back(n.y);
                vertices.push_back(n.z);
            }
        }

        for (unsigned i = 0; i != mesh->mNumFaces; i++)
        {
            for (unsigned j = 0; j != 3; j++)
                indices.push_back(mesh->mFaces[i].mIndices[j]);
        }
        aiReleaseImport(scene);

        const uint32_t vertexBufferSize = static_cast<uint32_t>(sizeof(float) * vertices.size());
        const uint32_t indexBufferSize = static_cast<uint32_t>(sizeof(unsigned int) * indices.size());

        return makeMeshBuffers(vertices, indices);
    }

    std::pair<BufferAttachment, BufferAttachment> Device::createPlaneBuffer_XZ(float sx, float sz)
    {
        return makeMeshBuffers(
            std::vector<float> {
            -sx, 0, -sz, 0, 0, 0, 1, 0,
                +sx, 0, -sz, 1, 0, 0, 1, 0,
                +sx, 0, +sz, 1, 1, 0, 1, 0,
                -sx, 0, +sz, 0, 1, 0, 1, 0
        },
            std::vector<unsigned int> { 0u, 1u, 2u, 0u, 3u, 2u });
    }

    std::pair<BufferAttachment, BufferAttachment> Device::createPlaneBuffer_XY(float sx, float sy)
    {
        return makeMeshBuffers(
            std::vector<float> {
            -sx, -sy, 0, 0, 0, 0, 0, 1,
                +sx, -sy, 0, 1, 0, 0, 0, 1,
                +sx, +sy, 0, 1, 1, 0, 0, 1,
                -sx, +sy, 0, 0, 1, 0, 0, 1
        },
            std::vector<unsigned int> { 0u, 1u, 2u, 0u, 3u, 2u });
    }
}
