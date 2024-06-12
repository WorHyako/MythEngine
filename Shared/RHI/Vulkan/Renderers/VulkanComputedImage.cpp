#include <RHI/Vulkan/Renderers/VulkanComputedImage.hpp>

ComputedImage::ComputedImage(VulkanRenderDevice& vkDev, const char* shaderName, uint32_t textureWidth, uint32_t textureHeight, bool supportDownload)
        : ComputedItem(vkDev, sizeof(uint32_t))
        , computedWidth(textureWidth)
        , computedHeight(textureHeight)
        , canDownloadImage(supportDownload)
{
    createComputedTexture(textureWidth, textureHeight);
    createComputedImageSetLayout();

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pNext = nullptr;
    pipelineLayoutInfo.flags = 0;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &dsLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    VK_CHECK(vkCreatePipelineLayout(vkDev.device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

    createDescriptorSet();

    ShaderModule s;
    createShaderModule(vkDev.device, &s, shaderName);
    if (createComputePipeline(vkDev.device, s.shaderModule, pipelineLayout, &pipeline) != VK_SUCCESS)
        exit(EXIT_FAILURE);

    vkDestroyShaderModule(vkDev.device, s.shaderModule, nullptr);
}

bool ComputedImage::createComputedTexture(uint32_t computedWidth, uint32_t computedHeight, VkFormat format)
{
    VkFormatProperties fmtProps;
    vkGetPhysicalDeviceFormatProperties(vkDev.physicalDevice, format, &fmtProps);
    if (!(fmtProps.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
        return false;

    if (!createImage(vkDev.device, vkDev.physicalDevice,
                     computedWidth, computedHeight, format,
                     VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | (canDownloadImage ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0),
                     !canDownloadImage ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                     computed.image, computed.imageMemory))
        return false;

    transitionImageLayout(vkDev, computed.image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    return createTextureSampler(vkDev.device, &computedImageSampler) &&
           createImageView(vkDev.device, computed.image, format, VK_IMAGE_ASPECT_COLOR_BIT, &computed.imageView);
}

bool ComputedImage::createComputedImageSetLayout()
{
    const std::vector<VkDescriptorPoolSize> poolSizes =
            {
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}
            };

    VkDescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.pNext = nullptr;
    descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptorPoolInfo.maxSets = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolInfo.pPoolSizes = poolSizes.data();

    VK_CHECK(vkCreateDescriptorPool(vkDev.device, &descriptorPoolInfo, nullptr, &descriptorPool));

    std::array<VkDescriptorSetLayoutBinding, 2> bindings =
            {
                    descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT),
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

bool ComputedImage::createDescriptorSet()
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &dsLayout;

    const auto res = vkAllocateDescriptorSets(vkDev.device, &allocInfo, &descriptorSet);
    VK_CHECK(res);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffer.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = uniformBuffer.size;
    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = computedImageSampler;
    imageInfo.imageView = computed.imageView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL/*VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL*/;

    VkWriteDescriptorSet imageDescriptorSet{};
    imageDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    imageDescriptorSet.pNext = nullptr;
    imageDescriptorSet.dstSet = descriptorSet;
    imageDescriptorSet.dstBinding = 0;
    imageDescriptorSet.dstArrayElement = 0;
    imageDescriptorSet.descriptorCount = 1;
    imageDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    imageDescriptorSet.pImageInfo = &imageInfo;
    imageDescriptorSet.pBufferInfo = nullptr;
    imageDescriptorSet.pTexelBufferView = nullptr;

    std::vector<VkWriteDescriptorSet> writeDescriptorSets =
            {
                    imageDescriptorSet,
                    bufferWriteDescriptorSet(descriptorSet, &bufferInfo, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            };

    vkUpdateDescriptorSets(vkDev.device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

    return true;
}

void ComputedImage::downloadImage(void* imageData)
{
    if (!canDownloadImage || !imageData)
        return;

    downloadImageData(vkDev, computed.image, computedWidth, computedHeight, VK_FORMAT_R8G8B8A8_UNORM, 1, imageData, VK_IMAGE_LAYOUT_GENERAL);
}