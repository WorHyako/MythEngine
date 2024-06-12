#include "VulkanQuadRenderer.hpp"

#include <Filesystem/FilesystemUtilities.hpp>

static constexpr int MAX_QUADS = 256;

void VulkanQuadRenderer::clear() { quads_.clear(); }

void VulkanQuadRenderer::quad(float x1, float y1, float x2, float y2)
{
    VertexData v1{{x1, y1, 0}, {0, 0}};
    VertexData v2{{x2, y1, 0}, {1, 0}};
    VertexData v3{{x2, y2, 0}, {1, 1}};
    VertexData v4{{x1, y2, 0}, {0, 1}};

    quads_.push_back(v1);
    quads_.push_back(v2);
    quads_.push_back(v3);

    quads_.push_back(v1);
    quads_.push_back(v3);
    quads_.push_back(v4);
}

bool VulkanQuadRenderer::createDescriptorSet(VulkanRenderDevice& vkDev)
{
    const std::array<VkDescriptorSetLayoutBinding, 3> bindings = {
        descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
        descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
        descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, static_cast<uint32_t>(textures_.size()))
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = nullptr;
    layoutInfo.flags = 0;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VK_CHECK(vkCreateDescriptorSetLayout(vkDev.device, &layoutInfo, nullptr, &descriptorSetLayout_));

    const std::vector<VkDescriptorSetLayout> layouts(vkDev.swapchainImages.size(), descriptorSetLayout_);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(vkDev.swapchainImages.size());
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets_.resize(vkDev.swapchainImages.size());

    VK_CHECK(vkAllocateDescriptorSets(vkDev.device, &allocInfo, descriptorSets_.data()));

    std::vector<VkDescriptorImageInfo> textureDescriptors(textures_.size());
    for (size_t i = 0; i < textures_.size(); i++)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = textureSamplers_[i];
        imageInfo.imageView = textures_[i].imageView;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        textureDescriptors[i] = imageInfo;
    }

    for (size_t i = 0; i < vkDev.swapchainImages.size(); i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers_[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(ConstBuffer);

        VkDescriptorBufferInfo bufferInfo2{};
        bufferInfo2.buffer = storageBuffers_[i];
        bufferInfo2.offset = 0;
        bufferInfo2.range = vertexBufferSize_;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets_[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.pBufferInfo = &bufferInfo;

        VkWriteDescriptorSet descriptorWrite2{};
        descriptorWrite2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite2.dstSet = descriptorSets_[i];
        descriptorWrite2.dstBinding = 1;
        descriptorWrite2.dstArrayElement = 0;
        descriptorWrite2.descriptorCount = 1;
        descriptorWrite2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrite2.pBufferInfo = &bufferInfo2;

        VkWriteDescriptorSet descriptorWrite3{};
        descriptorWrite3.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite3.dstSet = descriptorSets_[i];
        descriptorWrite3.dstBinding = 2;
        descriptorWrite3.dstArrayElement = 0;
        descriptorWrite3.descriptorCount = static_cast<uint32_t>(textures_.size());
        descriptorWrite3.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite3.pImageInfo = textureDescriptors.data();

        const std::array<VkWriteDescriptorSet, 3> descriptorWrites = {
            descriptorWrite,
            descriptorWrite2,
            descriptorWrite3
        };
        vkUpdateDescriptorSets(vkDev.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    return true;
}

void VulkanQuadRenderer::fillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage)
{
    if (quads_.empty())
        return;

    beginRenderPass(commandBuffer, currentImage);

    vkCmdDraw(commandBuffer, static_cast<uint32_t>(quads_.size()), 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
}

void VulkanQuadRenderer::updateBuffer(VulkanRenderDevice& vkDev, size_t i)
{
    uploadBufferData(vkDev, storageBuffersMemory_[i], 0, quads_.data(), quads_.size() * sizeof(VertexData));
}

void VulkanQuadRenderer::pushConstants(VkCommandBuffer commandBuffer, uint32_t textureIndex, const glm::vec2& offset)
{
    const ConstBuffer constBuffer = {offset, textureIndex};
    vkCmdPushConstants(commandBuffer, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ConstBuffer), &constBuffer);
}

VulkanQuadRenderer::VulkanQuadRenderer(VulkanRenderDevice& vkDev, const std::vector<std::string>& textureFiles)
    : vkDev(vkDev)
    , RendererBase(vkDev, VulkanImage())
{
    const size_t imgCount = vkDev.swapchainImages.size();

    framebufferWidth_ = vkDev.framebufferWidth;
    framebufferHeight_ = vkDev.framebufferHeight;

    storageBuffers_.resize(imgCount);
    storageBuffersMemory_.resize(imgCount);

    vertexBufferSize_ = MAX_QUADS * 6 * sizeof(VertexData);

    for (size_t i = 0; i < imgCount; i++)
    {
        if (!createBuffer(vkDev.device, vkDev.physicalDevice, vertexBufferSize_,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            storageBuffers_[i], storageBuffersMemory_[i]))
        {
            printf("Cannot create vertex buffer\n");
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
    }

    if (!createUniformBuffers(vkDev, sizeof(ConstBuffer)))
    {
        printf("Cannot create data buffers\n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    const size_t numTextureFiles = textureFiles.size();

    textures_.resize(numTextureFiles);
    textureSamplers_.resize(numTextureFiles);
    for (size_t i = 0; i < numTextureFiles; i++)
    {
        printf("\rLoading texture %u...", unsigned(i));
        createTextureImage(vkDev, textureFiles[i].c_str(), textures_[i].image, textures_[i].imageMemory);
        createImageView(vkDev.device, textures_[i].image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &textures_[i].imageView);
        createTextureSampler(vkDev.device, &textureSamplers_[i]);
    }
    printf("\n");

    if (!createDepthResources(vkDev, vkDev.framebufferWidth, vkDev.framebufferHeight, depthTexture_) ||
        !createDescriptorPool(vkDev, 1, 1, 1 * static_cast<uint32_t>(textures_.size()), &descriptorPool_) ||
        !createDescriptorSet(vkDev) ||
        !createColorAndDepthRenderPass(vkDev, false, &renderPass_, RenderPassCreateInfo()) ||
        !createPipelineLayoutWithConstants(vkDev.device, descriptorSetLayout_, &pipelineLayout_, sizeof(ConstBuffer), 0) ||
        !createGraphicsPipeline(vkDev, renderPass_, pipelineLayout_, 
            {
                (FilesystemUtilities::GetShadersDir() + "Vulkan/VK_texture_array.vert").c_str(),
                (FilesystemUtilities::GetShadersDir() + "Vulkan/VK_texture_array.frag").c_str()
            },
            &graphicsPipeline_))
    {
        printf("Failed to create pipeline\n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    createColorAndDepthFramebuffers(vkDev, renderPass_, VK_NULL_HANDLE /*depthTexture.imageView*/, swapchainFramebuffers_);
}

VulkanQuadRenderer::~VulkanQuadRenderer()
{
    VkDevice device = vkDev.device;

    for (size_t i = 0; i < storageBuffers_.size(); i++)
    {
        vkDestroyBuffer(device, storageBuffers_[i], nullptr);
        vkFreeMemory(device, storageBuffersMemory_[i], nullptr);
    }

    for (size_t i = 0; i < textures_.size(); i++)
    {
        vkDestroySampler(device, textureSamplers_[i], nullptr);
        destroyVulkanImage(device, textures_[i]);
    }

    destroyVulkanImage(device, depthTexture_);
}
