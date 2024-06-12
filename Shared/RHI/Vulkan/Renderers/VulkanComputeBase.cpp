#include <RHI/Vulkan/Renderers/VulkanComputeBase.hpp>

ComputeBase::ComputeBase(VulkanRenderDevice& vkDev, const char* shaderName, uint32_t inputSize, uint32_t outputSize)
	: vkDev(vkDev)
{
	createSharedBuffer(vkDev, inputSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		inBuffer, inBufferMemory);

	createSharedBuffer(vkDev, outputSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		outBuffer, outBufferMemory);

	ShaderModule s;
	createShaderModule(vkDev.device, &s, shaderName);

	createComputeDescriptorSetLayout(vkDev.device, &dsLayout);
	createPipelineLayout(vkDev.device, dsLayout, &pipelineLayout);
	createComputePipeline(vkDev.device, s.shaderModule, pipelineLayout, &pipeline);
	createComputeDescriptorSet(vkDev.device, dsLayout);

	vkDestroyShaderModule(vkDev.device, s.shaderModule, nullptr);
}

ComputeBase::~ComputeBase()
{
	vkDestroyBuffer(vkDev.device, inBuffer, nullptr);
	vkFreeMemory(vkDev.device, inBufferMemory, nullptr);

	vkDestroyBuffer(vkDev.device, outBuffer, nullptr);
	vkFreeMemory(vkDev.device, outBufferMemory, nullptr);

	vkDestroyPipelineLayout(vkDev.device, pipelineLayout, nullptr);
	vkDestroyPipeline(vkDev.device, pipeline, nullptr);

	vkDestroyDescriptorSetLayout(vkDev.device, dsLayout, nullptr);
	vkDestroyDescriptorPool(vkDev.device, descriptorPool, nullptr);
}

bool ComputeBase::createComputeDescriptorSet(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
{
	// Descriptor pool
	VkDescriptorPoolSize descriptorPoolSize = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 };

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.pNext = nullptr;
	descriptorPoolCreateInfo.flags = 0;
	descriptorPoolCreateInfo.maxSets = 1;
	descriptorPoolCreateInfo.poolSizeCount = 1;
	descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;

	VK_CHECK(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool));

	// Descriptor set
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.pNext = nullptr;
	descriptorSetAllocateInfo.descriptorPool = descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;

	VK_CHECK(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet));

	// Finally, update descriptor set with concrete buffer pointers
	VkDescriptorBufferInfo inBufferInfo = { inBuffer, 0, VK_WHOLE_SIZE };
	VkDescriptorBufferInfo outBufferInfo = { outBuffer, 0, VK_WHOLE_SIZE };

	VkWriteDescriptorSet inBufferSet{};
	inBufferSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	inBufferSet.pNext = nullptr;
	inBufferSet.dstSet = descriptorSet;
	inBufferSet.dstBinding = 0;
	inBufferSet.dstArrayElement = 0;
	inBufferSet.descriptorCount = 1;
	inBufferSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	inBufferSet.pImageInfo = nullptr;
	inBufferSet.pBufferInfo = &inBufferInfo;
	inBufferSet.pTexelBufferView = nullptr;
	VkWriteDescriptorSet outBufferSet{};
	outBufferSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	outBufferSet.pNext = nullptr;
	outBufferSet.dstSet = descriptorSet;
	outBufferSet.dstBinding = 1;
	outBufferSet.dstArrayElement = 0;
	outBufferSet.descriptorCount = 1;
	outBufferSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	outBufferSet.pImageInfo = nullptr;
	outBufferSet.pBufferInfo = &outBufferInfo;
	outBufferSet.pTexelBufferView = nullptr;

	VkWriteDescriptorSet writeDescriptorSet[2] = { inBufferSet, outBufferSet };

	vkUpdateDescriptorSets(device, 2, writeDescriptorSet, 0, nullptr);

	return true;
}