#include <RHI/RHIVulkan/VulkanBackend.hpp>

#include <EasyProfilerWrapper.hpp>

#include <assimp/scene.h>
#include <assimp/cimport.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>

namespace RHI::Vulkan
{
    static VkBufferUsageFlags pickBufferUsage(const BufferDesc& desc)
    {
        VkImageUsageFlags ret = 0;

        if (desc.usage.isTransferSrc)
            ret |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        if (desc.usage.isTransferDst)
            ret |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    	if (desc.usage.isVertexBuffer)
            ret |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        if (desc.usage.isIndexBuffer)
            ret |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        if (desc.usage.isUniformBuffer)
            ret |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        if (desc.usage.isStorageBuffer)
            ret |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        if (desc.usage.isDrawIndirectBuffer)
            ret |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

        return ret;
    }

	IBuffer* Device::createBuffer(const BufferDesc& desc)
    {
        Buffer* buffer = new Buffer(m_Context);

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = nullptr;
        bufferInfo.flags = 0;
        bufferInfo.size = desc.size;
        bufferInfo.usage = pickBufferUsage(desc);
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.queueFamilyIndexCount = 0;
        bufferInfo.pQueueFamilyIndices = nullptr;

        VK_CHECK(vkCreateBuffer(m_Context.device, &bufferInfo, nullptr, &buffer->buffer));

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_Context.device, buffer->buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, pickMemoryProperties(desc.memoryProperties));

        VK_CHECK(vkAllocateMemory(m_Context.device, &allocInfo, nullptr, &buffer->memory));

        vkBindBufferMemory(m_Context.device, buffer->buffer, buffer->memory, 0);

        return buffer;
    }

    IBuffer* Device::createSharedBuffer(const BufferDesc& desc)
    {
        uint32_t familyCount = static_cast<uint32_t>(m_DeviceQueueIndices.size());

        if (familyCount < 2)
            return createBuffer(desc);

        Buffer* buffer = new Buffer(m_Context);

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = nullptr;
        bufferInfo.flags = 0;
        bufferInfo.size = desc.size;
        bufferInfo.usage = pickBufferUsage(desc);
        bufferInfo.sharingMode = (familyCount > 1) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(m_DeviceQueueIndices.size());
        bufferInfo.pQueueFamilyIndices = (familyCount > 1) ? m_DeviceQueueIndices.data() : nullptr;

        VK_CHECK(vkCreateBuffer(m_Context.device, &bufferInfo, nullptr, &buffer->buffer));

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_Context.device, buffer->buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, pickMemoryProperties(desc.memoryProperties));

        VK_CHECK(vkAllocateMemory(m_Context.device, &allocInfo, nullptr, &buffer->memory));

        vkBindBufferMemory(m_Context.device, buffer->buffer, buffer->memory, 0);

        return buffer;
    }

    IBuffer* Device::createUniformBuffer(VkDeviceSize bufferSize)
    {
        BufferDesc desc = BufferDesc{}
    		.setSize(bufferSize)
            .setIsUniformBuffer(true)
            .setMemoryProperties(MemoryPropertiesBits::HOST_VISIBLE_BIT | MemoryPropertiesBits::HOST_COHERENT_BIT);
        return createBuffer(desc);
    }

    void Device::uploadBufferData(IBuffer* buffer, size_t deviceOffset, const void* data, const size_t dataSize)
    {
        EASY_FUNCTION()

    	Buffer* buf = dynamic_cast<Buffer*>(buffer);

    	void* mappedData = nullptr;
        vkMapMemory(m_Context.device, buf->memory, deviceOffset, dataSize, 0, &mappedData);
        memcpy(mappedData, data, dataSize);
        vkUnmapMemory(m_Context.device, buf->memory);
    }

    void Device::downloadBufferData(const VkDeviceMemory& bufferMemory, VkDeviceSize deviceOffset, void* outData, size_t dataSize)
    {
        EASY_FUNCTION()

    	void* mappedData = nullptr;
        vkMapMemory(m_Context.device, bufferMemory, deviceOffset, dataSize, 0, &mappedData);
        memcpy(outData, mappedData, dataSize);
        vkUnmapMemory(m_Context.device, bufferMemory);
    }

    IBuffer* Device::addBuffer(const BufferDesc& desc, bool createMapping)
    {
		Buffer* buffer = dynamic_cast<Buffer*>(createSharedBuffer(desc));
        if (!buffer)
        {
            printf("Cannot allocate buffer\n");
            exit(EXIT_FAILURE);
        }
        else
        {
            buffer->size = desc.size;
            //m_Resources.allBuffers.push_back(buffer);
        }

        if (createMapping)
            vkMapMemory(m_Context.device, buffer->memory, 0, VK_WHOLE_SIZE, 0, &buffer->ptr);

        return buffer;
    }

    IBuffer* Device::addVertexBuffer(uint32_t indexBufferSize, const void* indexData, uint32_t vertexBufferSize, const void* vertexData)
    {
        return allocateVertexBuffer(vertexBufferSize, vertexData, indexBufferSize, indexData);
        //m_Resources.allBuffers.push_back(result);
    }

    IBuffer* Device::allocateVertexBuffer(size_t vertexDataSize, const void* vertexData, size_t indexDataSize, const void* indexData)
    {
        VkDeviceSize bufferSize = vertexDataSize + indexDataSize;

        BufferDesc stagingDesc = BufferDesc{}
            .setSize(bufferSize)
            .setIsTransferSrc(true)
            .setMemoryProperties(MemoryPropertiesBits::HOST_VISIBLE_BIT | MemoryPropertiesBits::HOST_COHERENT_BIT);
        Buffer* stagingBuffer = dynamic_cast<Buffer*>(createBuffer(stagingDesc));

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
        Buffer* storageBuffer = dynamic_cast<Buffer*>(createBuffer(storageDesc));
        storageBuffer->size = bufferSize;

        // TODO:fix command list issue
        //copyBuffer(vkDev, stagingBuffer, *storageBuffer, bufferSize);

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

    Buffer::~Buffer()
	{
        vkDestroyBuffer(m_Context.device, buffer, nullptr);
        vkFreeMemory(m_Context.device, memory, nullptr);
	}
}
