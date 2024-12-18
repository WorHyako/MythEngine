#include <RHI/RenderUtils/Buffer/BufferUtils.hpp>

#include <assimp/scene.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/version.h>
#include <glad/vulkan.h>

namespace RenderUtils
{
	using namespace RHI;

    IBuffer* addVertexBuffer(IDevice* device, IRHICommandList* commandList, uint32_t indexBufferSize, const void* indexData, uint32_t vertexBufferSize, const void* vertexData)
    {
        return allocateVertexBuffer(device, commandList, vertexBufferSize, vertexData, indexBufferSize, indexData);
        //m_Resources.allBuffers.push_back(result);
    }

    IBuffer* allocateVertexBuffer(IDevice* device, IRHICommandList* commandList, size_t vertexDataSize, const void* vertexData, size_t indexDataSize, const void* indexData)
    {
        size_t bufferSize = vertexDataSize + indexDataSize;

        BufferDesc stagingDesc = BufferDesc{}
            .setSize(bufferSize)
            .setIsTransferSrc(true)
            .setMemoryProperties(MemoryPropertiesBits::HOST_VISIBLE_BIT | MemoryPropertiesBits::HOST_COHERENT_BIT);
        IBuffer* stagingBuffer = device->createBuffer(stagingDesc);

        device->uploadVertexIndexBufferData(stagingBuffer, 0, vertexDataSize, vertexData, indexDataSize, indexData, bufferSize);

        BufferDesc storageDesc = BufferDesc{}
            .setSize(bufferSize)
            .setIsTransferDst(true)
            .setIsStorageBuffer(true)
            .setMemoryProperties(MemoryPropertiesBits::DEVICE_LOCAL_BIT);
        IBuffer* storageBuffer = device->createBuffer(storageDesc);

        commandList->copyBuffer(stagingBuffer, storageBuffer, bufferSize);

        delete stagingBuffer;

        return storageBuffer;
    }

    /* Helper mesh-related functions */
    std::pair<BufferAttachment, BufferAttachment> makeMeshBuffers(IDevice* device, IRHICommandList* commandList, const std::vector<float>& vertices, const std::vector<unsigned>& indices)
    {
        const uint32_t indexBufferSize = uint32_t(indices.size() * sizeof(int));
        const uint32_t vertexBufferSize = uint32_t(vertices.size() * sizeof(float));

        IBuffer* storageBuffer = addVertexBuffer(device, commandList, indexBufferSize, indices.data(), vertexBufferSize, vertices.data());

        BufferAttachment vertexBufferAttachment{};
        vertexBufferAttachment.dInfo = { DescriptorType::STORAGE_BUFFER, ShaderStageFlagBits::VERTEX_BIT };
        vertexBufferAttachment.buffer = storageBuffer;
        vertexBufferAttachment.offset = 0;
        vertexBufferAttachment.size = vertexBufferSize;
        BufferAttachment indexBufferAttachment{};
        indexBufferAttachment.dInfo = { DescriptorType::STORAGE_BUFFER, ShaderStageFlagBits::VERTEX_BIT };
        indexBufferAttachment.buffer = storageBuffer;
        indexBufferAttachment.offset = vertexBufferSize;
        indexBufferAttachment.size = indexBufferSize;

        return std::pair{vertexBufferAttachment, indexBufferAttachment};
    }

    std::pair<BufferAttachment, BufferAttachment> loadMeshToBuffer(
        IDevice* device,
        IRHICommandList* commandList,
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
            IBuffer* nullBuffer = nullptr;

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

        return makeMeshBuffers(device, commandList, vertices, indices);
    }

    std::pair<BufferAttachment, BufferAttachment> createPlaneBuffer_XZ(IDevice* device, IRHICommandList* commandList, float sx, float sz)
    {
        return makeMeshBuffers(
            device, commandList,
            std::vector<float> {
            -sx, 0, -sz, 0, 0, 0, 1, 0,
                +sx, 0, -sz, 1, 0, 0, 1, 0,
                +sx, 0, +sz, 1, 1, 0, 1, 0,
                -sx, 0, +sz, 0, 1, 0, 1, 0
        },
            std::vector<unsigned int> { 0u, 1u, 2u, 0u, 3u, 2u });
    }

    std::pair<BufferAttachment, BufferAttachment> createPlaneBuffer_XY(IDevice* device, IRHICommandList* commandList, float sx, float sy)
    {
        return makeMeshBuffers(
            device, commandList,
            std::vector<float> {
            -sx, -sy, 0, 0, 0, 0, 0, 1,
                +sx, -sy, 0, 1, 0, 0, 0, 1,
                +sx, +sy, 0, 1, 1, 0, 0, 1,
                -sx, +sy, 0, 0, 1, 0, 0, 1
        },
            std::vector<unsigned int> { 0u, 1u, 2u, 0u, 3u, 2u });
    }
}
