#include <RHI/RHIVulkan/VulkanBackend.hpp>

#include <EasyProfilerWrapper.hpp>

#include <assimp/scene.h>
#include <assimp/cimport.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>

namespace RHI::Vulkan
{
    bool Device::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = nullptr;
        bufferInfo.flags = 0;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.queueFamilyIndexCount = 0;
        bufferInfo.pQueueFamilyIndices = nullptr;

        VK_CHECK(vkCreateBuffer(m_Context.device, &bufferInfo, nullptr, &buffer));

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_Context.device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        VK_CHECK(vkAllocateMemory(m_Context.device, &allocInfo, nullptr, &bufferMemory));

        vkBindBufferMemory(m_Context.device, buffer, bufferMemory, 0);

        return true;
    }

    bool Device::createSharedBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
    {
        uint32_t familyCount = static_cast<uint32_t>(m_DeviceQueueIndices.size());

        if (familyCount < 2)
            return createBuffer(size, usage, properties, buffer, bufferMemory);

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = nullptr;
        bufferInfo.flags = 0;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = (familyCount > 1) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(m_DeviceQueueIndices.size());
        bufferInfo.pQueueFamilyIndices = (familyCount > 1) ? m_DeviceQueueIndices.data() : nullptr;

        VK_CHECK(vkCreateBuffer(m_Context.device, &bufferInfo, nullptr, &buffer));

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_Context.device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        VK_CHECK(vkAllocateMemory(m_Context.device, &allocInfo, nullptr, &bufferMemory));

        vkBindBufferMemory(m_Context.device, buffer, bufferMemory, 0);

        return true;
    }

    bool Device::createUniformBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize bufferSize)
    {
        return createBuffer(bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            buffer, bufferMemory);
    }

    void Device::uploadBufferData(const VkDeviceMemory& bufferMemory, VkDeviceSize deviceOffset, const void* data, const size_t dataSize)
    {
        EASY_FUNCTION()

    	void* mappedData = nullptr;
        vkMapMemory(m_Context.device, bufferMemory, deviceOffset, dataSize, 0, &mappedData);
        memcpy(mappedData, data, dataSize);
        vkUnmapMemory(m_Context.device, bufferMemory);
    }

    void Device::downloadBufferData(const VkDeviceMemory& bufferMemory, VkDeviceSize deviceOffset, void* outData, size_t dataSize)
    {
        EASY_FUNCTION()

    	void* mappedData = nullptr;
        vkMapMemory(m_Context.device, bufferMemory, deviceOffset, dataSize, 0, &mappedData);
        memcpy(outData, mappedData, dataSize);
        vkUnmapMemory(m_Context.device, bufferMemory);
    }

    Buffer Device::addBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, bool createMapping)
    {
        Buffer buffer{};
        buffer.buffer = VK_NULL_HANDLE;
        buffer.size = 0;
        buffer.memory = VK_NULL_HANDLE;
        buffer.ptr = nullptr;

        if (!createSharedBuffer(size, usage, properties, buffer.buffer, buffer.memory))
        {
            printf("Cannot allocate buffer\n");
            exit(EXIT_FAILURE);
        }
        else
        {
            buffer.size = size;
            m_Resources.allBuffers.push_back(buffer);
        }

        if (createMapping)
            vkMapMemory(m_Context.device, buffer.memory, 0, VK_WHOLE_SIZE, 0, &buffer.ptr);

        return buffer;
    }

    Buffer Device::addVertexBuffer(uint32_t indexBufferSize, const void* indexData, uint32_t vertexBufferSize, const void* vertexData)
    {
        Buffer result;
        result.size = allocateVertexBuffer(&result.buffer, &result.memory, vertexBufferSize, vertexData, indexBufferSize, indexData);
        m_Resources.allBuffers.push_back(result);
        return result;
    }

    size_t Device::allocateVertexBuffer(VkBuffer* storageBuffer, VkDeviceMemory* storageBufferMemory, size_t vertexDataSize, const void* vertexData, size_t indexDataSize, const void* indexData)
    {
        VkDeviceSize bufferSize = vertexDataSize + indexDataSize;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(m_Context.device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertexData, vertexDataSize);
        memcpy((unsigned char*)data + vertexDataSize, indexData, indexDataSize);
        vkUnmapMemory(m_Context.device, stagingBufferMemory);

        createBuffer(
            bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *storageBuffer, *storageBufferMemory);

        copyBuffer(vkDev, stagingBuffer, *storageBuffer, bufferSize);

        vkDestroyBuffer(m_Context.device, stagingBuffer, nullptr);
        vkFreeMemory(m_Context.device, stagingBufferMemory, nullptr);

        return bufferSize;
    }

    /* Helper mesh-related functions */
    std::pair<BufferAttachment, BufferAttachment> Device::makeMeshBuffers(const std::vector<float>& vertices, const std::vector<unsigned>& indices)
    {
        const uint32_t indexBufferSize = uint32_t(indices.size() * sizeof(int));
        const uint32_t vertexBufferSize = uint32_t(vertices.size() * sizeof(float));

        Buffer storageBuffer = addVertexBuffer(indexBufferSize, indices.data(), vertexBufferSize, vertices.data());

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
            Buffer nullBuffer{ VK_NULL_HANDLE, 0, VK_NULL_HANDLE };

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
