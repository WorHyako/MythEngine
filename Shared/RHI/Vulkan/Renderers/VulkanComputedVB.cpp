#include <RHI/Vulkan/Renderers/VulkanComputedVB.hpp>

ComputedVertexBuffer::ComputedVertexBuffer(VulkanRenderDevice& vkDev, const char* shaderName,
    uint32_t indexBufferSize,
    uint32_t uniformBufferSize,
    uint32_t vertexSize,
    uint32_t vertexCount,
    bool supportDownload)
    : ComputedItem(vkDev, uniformBufferSize)
    , computedVertexCount(vertexCount)
    , indexBufferSize(indexBufferSize)
    , vertexSize(vertexSize)
	, canDownloadVertices(supportDownload)
{
    createComputedBuffer();
    createComputedImageSetLayout();
    createPipelineLayout(vkDev.device, dsLayout, &pipelineLayout);

    createDescriptorSet();

    ShaderModule s;
    createShaderModule(vkDev.device, &s, shaderName);
    if (createComputePipeline(vkDev.device, s.shaderModule, pipelineLayout, &pipeline) != VK_SUCCESS)
        exit(EXIT_FAILURE);

    vkDestroyShaderModule(vkDev.device, s.shaderModule, nullptr);
}

bool ComputedVertexBuffer::createComputedBuffer()
{
    return createBuffer(vkDev.device, vkDev.physicalDevice,
        computedVertexCount * vertexSize + indexBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | (canDownloadVertices ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 0) | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        !canDownloadVertices ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
        computedBuffer, computedMemory);
}

bool ComputedVertexBuffer::createComputedImageSetLayout()
{
    std::vector<VkDescriptorPoolSize> poolSizes =
    {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}
    };

    VkDescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.pNext = nullptr;
    descriptorPoolInfo.flags = 0;
    descriptorPoolInfo.maxSets = 1;
    descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolInfo.pPoolSizes = poolSizes.data();

    VK_CHECK(vkCreateDescriptorPool(vkDev.device, &descriptorPoolInfo, nullptr, &descriptorPool));

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
        descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT),
        descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = nullptr;
    layoutInfo.flags = 0;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VK_CHECK(vkCreateDescriptorSetLayout(vkDev.device, &layoutInfo, nullptr, &dsLayout));

    return true;
}

bool ComputedVertexBuffer::createDescriptorSet()
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &dsLayout;

    VK_CHECK(vkAllocateDescriptorSets(vkDev.device, &allocInfo, &descriptorSet));

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = computedBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = computedVertexCount * vertexSize;
    VkDescriptorBufferInfo bufferInfo2{};
    bufferInfo2.buffer = uniformBuffer.buffer;
    bufferInfo2.offset = 0;
    bufferInfo2.range = uniformBuffer.size;

    std::vector<VkWriteDescriptorSet> writeDescriptorSets =
    {
        bufferWriteDescriptorSet(descriptorSet, &bufferInfo, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
        bufferWriteDescriptorSet(descriptorSet, &bufferInfo2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
    };

    vkUpdateDescriptorSets(vkDev.device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

    return true;
}

void ComputedVertexBuffer::downloadVertices(void* vertexData)
{
    if (!canDownloadVertices || !vertexData)
        return;

    downloadBufferData(vkDev, computedMemory, 0, vertexData, computedVertexCount * vertexSize);
}

void ComputedVertexBuffer::uploadIndexData(uint32_t* indices)
{
    uploadBufferData(vkDev, computedMemory, computedVertexCount * vertexSize, indices, indexBufferSize);
}