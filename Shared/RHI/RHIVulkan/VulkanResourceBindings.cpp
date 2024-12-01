#include <RHI/RHIVulkan/VulkanBackend.hpp>

namespace RHI::Vulkan
{
    VkDescriptorPool Device::addDescriptorPool(const DescriptorSetInfo& dsInfo, uint32_t dSetCount)
    {
        uint32_t uniformBufferCount = 0;
        uint32_t storageBufferCount = 0;
        uint32_t samplerCount = static_cast<uint32_t>(dsInfo.textures.size());

        for (const auto& ta : dsInfo.textureArrays)
            samplerCount += static_cast<uint32_t>(ta.textures.size());

        for (const auto& b : dsInfo.buffers)
        {
            if (b.dInfo.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                uniformBufferCount++;
            if (b.dInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                storageBufferCount++;
        }

        std::vector<VkDescriptorPoolSize> poolSizes;

        /* printf("Allocating pool[%d | %d | %d]\n", (int)uniformBufferCount, (int)storageBufferCount, (int)samplerCount); */

        if (uniformBufferCount)
            poolSizes.push_back(VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, dSetCount * uniformBufferCount });

        if (storageBufferCount)
            poolSizes.push_back(VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, dSetCount * storageBufferCount });

        if (samplerCount)
            poolSizes.push_back(VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, dSetCount * samplerCount });

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.pNext = nullptr;
        poolInfo.flags = 0;
        poolInfo.maxSets = static_cast<uint32_t>(dSetCount);
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.empty() ? nullptr : poolSizes.data();

        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

        if (vkCreateDescriptorPool(m_Context.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        {
            printf("Cannot allocate descriptor pool\n");
            exit(EXIT_FAILURE);
        }

        m_Resources.allDPools.push_back(descriptorPool);
        return descriptorPool;
    }


    VkDescriptorSetLayout Device::addDescriptorSetLayout(const DescriptorSetInfo& dsInfo)
    {
        VkDescriptorSetLayout descriptorSetLayout;

        uint32_t bindingIdx = 0;

        std::vector<VkDescriptorSetLayoutBinding> bindings;

        for (const auto& b : dsInfo.buffers)
        {
            bindings.push_back(descriptorSetLayoutBinding(bindingIdx++, b.dInfo.type, b.dInfo.shaderStageFlags));
        }

        for (const auto& i : dsInfo.textures)
        {
            bindings.push_back(descriptorSetLayoutBinding(bindingIdx++, i.dInfo.type /*VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER*/, i.dInfo.shaderStageFlags));
        }

        for (const auto& t : dsInfo.textureArrays)
        {
            bindings.push_back(descriptorSetLayoutBinding(bindingIdx++, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, t.dInfo.shaderStageFlags, static_cast<uint32_t>(t.textures.size())));
        }

        /*const VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
            .bindingCount = static_cast<uint32_t>(descriptorBindingFlags.size()),
            .pBindingFlags = descriptorBindingFlags.data() };*/


        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.pNext = nullptr; // dsInfo.textureArrays.empty() ? nullptr : &setLayoutBindingFlags
        layoutInfo.flags = 0;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.size() > 0 ? bindings.data() : nullptr;

        if (vkCreateDescriptorSetLayout(m_Context.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        {
            printf("Failed to create descriptor set layout\n");
            exit(EXIT_FAILURE);
        }

        m_Resources.allDSLayouts.push_back(descriptorSetLayout);
        return descriptorSetLayout;
    }

    VkDescriptorSet Device::addDescriptorSet(VkDescriptorPool descriptorPool, VkDescriptorSetLayout dsLayout)
    {
        VkDescriptorSet descriptorSet;

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &dsLayout;

        if (vkAllocateDescriptorSets(m_Context.device, &allocInfo, &descriptorSet) != VK_SUCCESS)
        {
            printf("Cannot allocate descriptor set\n");
            exit(EXIT_FAILURE);
        }

        return descriptorSet;
    }

    /*
        This routine counts all textures in all texture arrays (if any of them are present),
        creates a list of DescriptorWrite operations with required buffer/image info structures
        and calls the vkUpdateDescriptorSets()
    */
    void Device::updateDescriptorSet(VkDescriptorSet ds, const DescriptorSetInfo& dsInfo)
    {
        uint32_t bindingIdx = 0;
        std::vector<VkWriteDescriptorSet> descriptorWrites;

        std::vector<VkDescriptorBufferInfo> bufferDescriptors(dsInfo.buffers.size());
        std::vector<VkDescriptorImageInfo> imageDescriptors(dsInfo.textures.size());
        std::vector<VkDescriptorImageInfo> imageArrayDescriptors;

        for (size_t i = 0; i < dsInfo.buffers.size(); i++)
        {
            BufferAttachment b = dsInfo.buffers[i];

            bufferDescriptors[i] = VkDescriptorBufferInfo{
                b.buffer.buffer,
                b.offset,
                (b.size > 0) ? b.size : VK_WHOLE_SIZE
            };

            descriptorWrites.push_back(bufferWriteDescriptorSet(ds, &bufferDescriptors[i], bindingIdx++, b.dInfo.type));
        }

        for (size_t i = 0; i < dsInfo.textures.size(); i++)
        {
            VulkanTexture t = dsInfo.textures[i].texture;

            imageDescriptors[i] = VkDescriptorImageInfo{
                t.sampler,
                t.image.imageView,
                /* t.texture.layout */ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };

            descriptorWrites.push_back(imageWriteDescriptorSet(ds, &imageDescriptors[i], bindingIdx++));
        }

        uint32_t taOffset = 0;
        std::vector<uint32_t> taOffsets(dsInfo.textureArrays.size());
        for (size_t ta = 0; ta < dsInfo.textureArrays.size(); ta++)
        {
            taOffsets[ta] = taOffset;

            for (size_t j = 0; j < dsInfo.textureArrays[ta].textures.size(); j++)
            {
                VulkanTexture t = dsInfo.textureArrays[ta].textures[j];

                VkDescriptorImageInfo imageInfo = {
                    t.sampler,
                    t.image.imageView,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                };

                imageArrayDescriptors.push_back(imageInfo); // item 'taOffsets[ta] + j'
            }

            taOffset += static_cast<uint32_t>(dsInfo.textureArrays[ta].textures.size());
        }

        for (size_t ta = 0; ta < dsInfo.textureArrays.size(); ta++)
        {
            VkWriteDescriptorSet writeSet{};
            writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeSet.pNext = nullptr;
            writeSet.dstSet = ds;
            writeSet.dstBinding = bindingIdx++;
            writeSet.dstArrayElement = 0;
            writeSet.descriptorCount = static_cast<uint32_t>(dsInfo.textureArrays[ta].textures.size());
            writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeSet.pImageInfo = imageArrayDescriptors.data() + taOffsets[ta];

            descriptorWrites.push_back(writeSet);
        }

        vkUpdateDescriptorSets(m_Context.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}
