#pragma once

#include <vector>
#include <functional>

#define VK_NO_PROTOTYPES
#include "volk.h"

#define GLFW_INCLUDE_VULKAN
#include <map>
#include <array>
#include <memory>
#include <string>

#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>

#include <GLFW/glfw3.h>

#include "RHICommon.hpp"

#define VK_CHECK(value) CHECK(value == VK_SUCCESS, __FILE__, __LINE__);
#define VK_CHECK_RET(value) if(value != VK_SUCCESS) { CHECK(false, __FILE__, __LINE__); return value; }
#define BL_CHECK(value) CHECK(value, __FILE__, __LINE__);

namespace RHI::Vulkan
{
	class Device;
	class CommandList;

	// Features we need for our Vulkan context
	struct VulkanContextFeatures
	{
		bool supportsScreenshots_ = false;

		/* for wireframe outlines */
		bool geometryShader_ = true;
		/* for tesselation experiments */
		bool tessellationShader_ = false;

		/* for indirect instanced rendering */
		bool multiDrawIndirect = false;
		bool drawIndirectFirstInstance = false;

		/* for OIT and general atomic operations */
		bool vertexPipelineStoresAndAtomics_ = false;
		bool fragmentStoresAndAtomics_ = false;

		/* for arrays of textures */
		bool shaderSampledImageArrayDynamicIndexing = false;

		/* for GL <-> VK material shader compatibility */
		bool shaderInt64 = false;

		bool deviceDescriptorIndexing = false;
	};

	struct VulkanContextExtensions
	{
		bool KHR_swapchain = false;
		bool KHR_maintenance3 = false;
		bool EXT_discriptor_indexing = false;
		bool EXT_draw_indirect_count = false;
#if defined (__APPLE__)
		bool KHR_portability_subset = false; // either KHR_ or Vulkan 1.2 versions
#endif
	};

	struct VulkanInstance final
	{
		VkInstance instance;
		VkSurfaceKHR surface;
		VkDebugUtilsMessengerEXT messenger;
		VkDebugReportCallbackEXT reportCallback;
	};

	struct VulkanContext
	{
		VulkanContext(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VulkanContextExtensions contextExtensions, VulkanContextFeatures contextFeatures)
			: instance(instance)
			, physicalDevice(physicalDevice)
			, device(device)
			, ctxExtensions(contextExtensions)
			, ctxFeatures(contextFeatures)
		{
		}

		void updateBuffers(uint32_t imageIndex);
		void composeFrame(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		// For Chapter 8 & 9
		inline PipelineInfo pipelineParametersForOutputs(const std::vector<VulkanTexture>& outputs) const
		{
			PipelineInfo pInfo{};
			pInfo.width = outputs.empty() ? vkDev.framebufferWidth : outputs[0].width;
			pInfo.height = outputs.empty() ? vkDev.framebufferHeight : outputs[0].height;
			pInfo.useBlending = false;
			return pInfo;
		}

		std::vector<VkFramebuffer> swapchainFramebuffers;
		std::vector<VkFramebuffer> swapchainFramebuffers_NoDepth;

		void beginRenderPass(VkCommandBuffer cmdBuffer, VkRenderPass pass, size_t currentImage, const VkRect2D area,
			VkFramebuffer fb = VK_NULL_HANDLE,
			uint32_t clearValueCount = 0, const VkClearValue* clearValues = nullptr)
		{
			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = pass;
			renderPassInfo.framebuffer = (fb != VK_NULL_HANDLE) ? fb : swapchainFramebuffers[currentImage];
			renderPassInfo.renderArea = area;
			renderPassInfo.clearValueCount = clearValueCount;
			renderPassInfo.pClearValues = clearValues;

			vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		}

		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		VkQueue graphicsQueue;

		VulkanInstance vulkanInstance;
		//Device vkDev;
		VulkanContextExtensions ctxExtensions;
		VulkanContextFeatures ctxFeatures;
	};

	class Queue
	{
	public:
		Queue(const VulkanContext& context, CommandQueue queueID, VkQueue queue, uint32_t queueFamilyIndex);
		~Queue();

		VkSemaphore semaphore;
		VkSemaphore renderSemaphore;

		CommandQueue getQueueID() const { return m_QueueID; }
		uint32_t getQueueFamilyIndex() const { return m_QueueFamilyIndex; }
		VkQueue getVkQueue() const { return m_Queue; }

	private:
		const VulkanContext& m_Context;

		VkQueue m_Queue;
		CommandQueue m_QueueID;
		uint32_t m_QueueFamilyIndex = uint32_t(-1);

	};

	class VulkanRHIModule : public IRHIModule
	{
	public:
		VulkanRHIModule();
		~VulkanRHIModule() override = default;

		virtual IDynamicRHI* createRHI() override;

		GLFWwindow* getWindowInterface() override
		{
			return window_;
		}

	private:
		GLFWwindow* window_;
	};

	struct DeviceDesc
	{
		uint32_t framebufferWidth;
		uint32_t framebufferHeight;

		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkDevice device;

		VulkanContextExtensions* ctxExtensions;
		VulkanContextFeatures* ctxFeatures;

		uint32_t graphicsFamily;
		VkQueue graphicsQueue;
		bool useGraphicsQueue = false;

		uint32_t computeFamily;
		VkQueue computeQueue;
		bool useComputeQueue = false;

		VkSwapchainKHR swapchain;
		VkSemaphore semaphore;
		VkSemaphore renderSemaphore;

		std::vector<VkImage> swapchainImages;
		std::vector<VkImageView> swapchainImageViews;

		VkCommandPool commandPool;
		std::vector<VkCommandBuffer> commandBuffers;

		VkCommandBuffer computeCommandBuffer;
		VkCommandPool computeCommandPool;
	};

	/* A structure with pipeline parameters */
	struct PipelineInfo
	{
		uint32_t width = 0;
		uint32_t height = 0;

		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; /* defaults to triangles*/

		bool useDepth = true;

		bool useBlending = true;

		bool dynamicScissorState = false;

		uint32_t patchControlPoints = 0;
	};

	struct VulkanDynamicRHI : public IDynamicRHI
	{
		VulkanDynamicRHI(GLFWwindow* window);
		~VulkanDynamicRHI();

		void createInstance();
		void destroyVulkanInstance(VulkanInstance& vk);

		static VulkanContextFeatures& initializeContextFeatures();
		static VulkanContextExtensions& initializeContextExtensions();

		VulkanInstance vk;
	private:
		GLFWwindow* window_;
	};

	struct SwapchainSupportDetails final
	{
		VkSurfaceCapabilitiesKHR capabilities = {};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct ShaderModule final : public IShader
	{
		std::vector<unsigned int> SPIRV;
		VkShaderModule shaderModule = nullptr;
	};

	struct VulkanBuffer : public IBuffer
	{
		VkBuffer		buffer;
		VkDeviceSize	size;
		VkDeviceMemory	memory;

		/* Permanent mapping to CPU address space (see VulkanResources::addBuffer) */
		void* ptr;
	};

	struct VulkanImage final : public IImage
	{
		VkImage image = nullptr;
		VkDeviceMemory imageMemory = nullptr;
		VkImageView imageView = nullptr;
	};

	// Aggregate structure for passing around the texture data
	struct VulkanTexture final : public ITexture
	{
		uint32_t width;
		uint32_t height;
		uint32_t depth;
		VkFormat format;

		VulkanImage image;
		VkSampler sampler;

		// Offscreen buffers require VK_IMAGE_LAYOUT_GENERAL && static textures have VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		VkImageLayout desiredLayout;
	};

	struct DescriptorInfo
	{
		VkDescriptorType type;
		VkShaderStageFlags shaderStageFlags;
	};

	struct BufferAttachment
	{
		DescriptorInfo  dInfo;

		VulkanBuffer    buffer;
		uint32_t        offset;
		uint32_t        size;
	};

	struct TextureAttachment
	{
		DescriptorInfo  dInfo;

		VulkanTexture   texture;
	};

	struct TextureArrayAttachment
	{
		DescriptorInfo  dInfo;

		std::vector<VulkanTexture>  textures;
	};

	inline TextureAttachment makeTextureAttachment(VulkanTexture tex, VkShaderStageFlags shaderStageFlags)
	{
		TextureAttachment textureAttachment{};
		textureAttachment.dInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textureAttachment.dInfo.shaderStageFlags = shaderStageFlags;
		textureAttachment.texture = tex;

		return textureAttachment;
	}

	inline TextureAttachment fsTextureAttachment(VulkanTexture tex)
	{
		return makeTextureAttachment(tex, VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	inline TextureArrayAttachment fsTextureArrayAttachment(const std::vector<VulkanTexture>& textures)
	{
		TextureArrayAttachment textureArrayAttachment{};
		textureArrayAttachment.dInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textureArrayAttachment.dInfo.shaderStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		textureArrayAttachment.textures = textures;

		return textureArrayAttachment;
	}

	inline BufferAttachment makeBufferAttachment(VulkanBuffer buffer, uint32_t offset, uint32_t size, VkDescriptorType type, VkShaderStageFlags shaderStageFlags)
	{
		BufferAttachment bufferAttachment{};
		bufferAttachment.dInfo = { type, shaderStageFlags };
		bufferAttachment.buffer = { buffer.buffer, buffer.size, buffer.memory, buffer.memory };
		bufferAttachment.offset = offset;
		bufferAttachment.size = size;
		return bufferAttachment;
	}

	inline BufferAttachment uniformBufferAttachment(VulkanBuffer buffer, uint32_t offset, uint32_t size, VkShaderStageFlags shaderStageFlags)
	{
		return makeBufferAttachment(buffer, offset, size, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, shaderStageFlags);
	}

	inline BufferAttachment storageBufferAttachment(VulkanBuffer buffer, uint32_t offset, uint32_t size, VkShaderStageFlags shaderStageFlags)
	{
		return makeBufferAttachment(buffer, offset, size, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, shaderStageFlags);
	}

	/** An aggregate structure with all the data for descriptor set (or descriptor set layout) allocation */
	struct DescriptorSetInfo
	{
		std::vector<BufferAttachment>       buffers;
		std::vector<TextureAttachment>      textures;
		std::vector<TextureArrayAttachment> textureArrays;
	};

	void CHECK(bool check, const char* fileName, int lineNumber);

	bool setupDebugCallbacks(VkInstance instance, VkDebugUtilsMessengerEXT* messenger, VkDebugReportCallbackEXT* reportCallback);

	size_t compileShaderFile(const char* file, ShaderModule& shaderModule);

	inline VkPipelineShaderStageCreateInfo shaderStageInfo(VkShaderStageFlagBits shaderStage, ShaderModule& module, const char* entryPoint)
	{
		VkPipelineShaderStageCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.stage = shaderStage;
		createInfo.module = module.shaderModule;
		createInfo.pName = entryPoint;
		createInfo.pSpecializationInfo = nullptr;
		return createInfo;
	}

	inline VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t descriptorCount = 1)
	{
		return VkDescriptorSetLayoutBinding{
			binding,
			descriptorType,
			descriptorCount,
			stageFlags,
			nullptr
		};
	}

	inline VkWriteDescriptorSet bufferWriteDescriptorSet(VkDescriptorSet ds, const VkDescriptorBufferInfo* bi, uint32_t bindIdx, VkDescriptorType dType)
	{
		VkWriteDescriptorSet descriptorSet{};
		descriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorSet.pNext = nullptr;
		descriptorSet.dstSet = ds;
		descriptorSet.dstBinding = bindIdx;
		descriptorSet.dstArrayElement = 0;
		descriptorSet.descriptorCount = 1;
		descriptorSet.descriptorType = dType;
		descriptorSet.pImageInfo = nullptr;
		descriptorSet.pBufferInfo = bi;
		descriptorSet.pTexelBufferView = nullptr;
		return descriptorSet;
	}

	inline VkWriteDescriptorSet imageWriteDescriptorSet(VkDescriptorSet ds, const VkDescriptorImageInfo* ii, uint32_t bindIdx)
	{
		VkWriteDescriptorSet descriptorSet{};
		descriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorSet.pNext = nullptr;
		descriptorSet.dstSet = ds;
		descriptorSet.dstBinding = bindIdx;
		descriptorSet.dstArrayElement = 0;
		descriptorSet.descriptorCount = 1;
		descriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorSet.pImageInfo = ii;
		descriptorSet.pBufferInfo = nullptr;
		descriptorSet.pTexelBufferView = nullptr;
		return descriptorSet;
	}

	VkResult createSwapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t graphicsFamily, uint32_t width, uint32_t height, VkSwapchainKHR* swapchain, bool supportScreenshots = false);

	size_t createSwapchainImages(VkDevice device, VkSwapchainKHR swapchain, std::vector<VkImage>& swapchainImages, std::vector<VkImageView>& swapchainImageViews);

	VkResult createSemaphore(VkDevice device, VkSemaphore* outSemaphore);

	bool createDescriptorPool(Device& vkDev, uint32_t uniformBufferCount, uint32_t storageBufferCount, uint32_t samplerCount, VkDescriptorPool* descriptorPool);

	bool isDeviceSuitable(VkPhysicalDevice device);

	SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	uint32_t chooseSwapImageCount(const VkSurfaceCapabilitiesKHR& caps);

	VkResult findSuitablePhysicalDevice(VkInstance instance, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDevice* physicalDevice);

	uint32_t findQueueFamilies(VkPhysicalDevice device, VkQueueFlags desiredFlags);

	bool createGraphicsPipeline(
		Device& vkDev,
		VkRenderPass renderPass, VkPipelineLayout pipelineLayout,
		const std::vector<const char*>& shaderFiles,
		VkPipeline* pipeline,
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST /* defaults to triangles*/,
		bool useDepth = true,
		bool useBlending = true,
		bool dynamicScissorState = false,
		int32_t customWidth = -1,
		int32_t customHeight = -1,
		uint32_t numPatchControlPoints = 0);

	enum eRenderPassBit : uint8_t
	{
		eRenderPassBit_First = 0x01, // clear the attachment
		eRenderPassBit_Last = 0x02, // transition to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		eRenderPassBit_Offscreen = 0x04, // transition to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		eRenderPassBit_OffscreenInternal = 0x08, // keepVK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
	};

	struct RenderPassCreateInfo final
	{
		bool clearColor_ = false;
		bool clearDepth_ = false;
		uint8_t flags_ = 0;
	};

	// Utility structure for Renderer classes to know the details about starting this pass
	struct RenderPass
	{
		RenderPass() = default;
		explicit RenderPass(bool useDepth = true, const RenderPassCreateInfo& ci = RenderPassCreateInfo());

		RenderPassCreateInfo info;
		VkRenderPass handle = VK_NULL_HANDLE;
	};

	uint32_t bytesPerTexFormat(VkFormat fmt);

	bool downloadImageData(Device& vkDev, VkImage& textureImage, uint32_t texWidth, uint32_t texHeight, VkFormat texFormat, uint32_t layerCount, void* imageData, VkImageLayout sourceImageLayout);

	bool createDepthResources(Device& vkDev, uint32_t width, uint32_t height, VulkanImage& depth);

	bool createTexturedVertexBuffer(Device& vkDev, const char* filename, VkBuffer* storageBuffer, VkDeviceMemory* storageBufferMemory, size_t* vertexBufferSize, size_t* indexBufferSize);

	bool createPBRVertexBuffer(Device& vkDev, const char* filename, VkBuffer* storageBuffer, VkDeviceMemory* storageBufferMemory, size_t* vertexBufferSize, size_t* indexBufferSize);

	bool executeComputeShader(Device& vkDev,
		VkPipeline computePipeline, VkPipelineLayout pl, VkDescriptorSet ds,
		uint32_t xsize, uint32_t ysize, uint32_t zsize);

	bool createComputeDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout* descriptorSetLayout);

	void insertComputedImageBarrier(VkCommandBuffer commandBuffer, VkImage image);


	/* Check if the texture is used as a depth buffer */
	inline bool isDepthFormat(VkFormat fmt) {
		return
			(fmt == VK_FORMAT_D16_UNORM) ||
			(fmt == VK_FORMAT_X8_D24_UNORM_PACK32) ||
			(fmt == VK_FORMAT_D32_SFLOAT) ||
			(fmt == VK_FORMAT_D16_UNORM_S8_UINT) ||
			(fmt == VK_FORMAT_D24_UNORM_S8_UINT) ||
			(fmt == VK_FORMAT_D32_SFLOAT_S8_UINT);
	}

	bool setVkObjectName(Device& vkDev, void* object, VkObjectType objectType, const char* name);

	inline bool setVkImageName(Device& vkDev, void* object, const char* name)
	{
		return setVkObjectName(vkDev, object, VK_OBJECT_TYPE_IMAGE, name);
	}

	/* This routine updates one texture discriptor in one descriptor set */
	void updateTextureInDescriptorSetArray(Device& vkDev, VkDescriptorSet ds, VulkanTexture t, uint32_t textureIndex, uint32_t bindingIdx);

	VkShaderStageFlagBits glslangShaderStageToVulkan(glslang_stage_t sh);
	glslang_stage_t glslangShaderStageFromFileName(const char* fileName);

	bool hasStencilComponent(VkFormat format);

	struct VulkanResources
	{
		VulkanResources(Device* device)
			: m_Device(device)
		{}
		~VulkanResources();

		std::vector<VulkanTexture> allTextures;
		std::vector<VulkanBuffer> allBuffers;

		std::vector<VkFramebuffer> allFramebuffers;
		std::vector<VkRenderPass> allRenderPasses;

		std::vector<VkPipelineLayout> allPipelineLayouts;
		std::vector<VkPipeline> allPipelines;

		std::vector<VkDescriptorSetLayout> allDSLayouts;
		std::vector<VkDescriptorPool> allDPools;

		std::vector<ShaderModule> shaderModules;
		std::map<std::string, uint32_t> shaderMap;

	private:
		Device* m_Device;
	};

	class Device final : public IDevice
	{
	public:
		Device(DeviceDesc& desc);
		virtual ~Device();

		Queue* getQueue(CommandQueue queue) const { return m_Queues[int(queue)].get(); }

		VkPhysicalDeviceFeatures initVulkanRenderDeviceFeatures(const VulkanContextFeatures& ctxFeatures, VkPhysicalDeviceFeatures2& deviceFeatures2);
		bool initVulkanRenderDevice(
			DeviceDesc& desc,
			std::function<bool(VkPhysicalDevice)> selector,
			VkPhysicalDeviceFeatures deviceFeatures,
			VkPhysicalDeviceFeatures2 deviceFeatures2);
		VkResult createDevice(VkPhysicalDeviceFeatures deviceFeatures, VkPhysicalDeviceFeatures2 deviceFeatures2);

		void destroyVulkanRenderDevice();

		/* Resource Management*/
		VulkanTexture loadTexture2D(const char* filename);

		VulkanTexture loadCubemap(const char* fileName, uint32_t mipLevels = 1);

		VulkanTexture loadKTX(const char* fileName);

		VulkanTexture createFontTexture(const char* fontFile);

		VulkanTexture addColorTexture(int texWidth = 0, int texHeight = 0,
			VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM,
			VkFilter minFilter = VK_FILTER_LINEAR, VkFilter maxFilter = VK_FILTER_LINEAR,
			VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

		VulkanTexture addDepthTexture(int texWidth = 0, int texHeight = 0, VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		VulkanTexture addSolidRGBATexture(uint32_t color = 0xFFFFFFFF);

		VulkanTexture addRGBATexture(int texWidth, int texHeight, void* data);

		bool createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkImageCreateFlags flags = 0, uint32_t mipLevels = 1);

		bool createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView* imageView, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, uint32_t layerCount = 1, uint32_t mipLevels = 1);

		bool createTextureSampler(VkSampler* sampler, VkFilter minFilter = VK_FILTER_LINEAR, VkFilter magFilter = VK_FILTER_LINEAR, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

		bool createDepthSampler(VkSampler* sampler);

		bool createOffscreenImage(
			VkImage& textureImage, VkDeviceMemory& textureImageMemory,
			uint32_t texWidth, uint32_t texHeight,
			VkFormat texFormat,
			uint32_t layerCount, VkImageCreateFlags flags);

		void destroyVulkanImage(VulkanImage& image);
		void destroyVulkanTexture(VulkanTexture& texture);

		bool createTextureImage(const char* filename, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t* outTexWidth = nullptr, uint32_t* outTexHeight = nullptr);

		bool createMIPTextureImage(const char* filename, uint32_t mipLevels, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t* width = nullptr, uint32_t* height = nullptr);

		bool createCubeTextureImage(const char* filename, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t* width = nullptr, uint32_t* height = nullptr);

		bool createMIPCubeTextureImage(const char* filename, uint32_t mipLevels, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t* width = nullptr, uint32_t* height = nullptr);

		bool createTextureImageFromData(
			VkImage& textureImage, VkDeviceMemory& textureImageMemory,
			void* imageData, uint32_t texWidth, uint32_t texHeight,
			VkFormat texFormat,
			uint32_t layerCount = 1, VkImageCreateFlags flags = 0);

		bool createMIPTextureImageFromData(
			VkImage& textureImage, VkDeviceMemory& textureImageMemory,
			void* mipData, uint32_t mipLevels, uint32_t texWidth, uint32_t texHeight,
			VkFormat texFormat,
			uint32_t layerCount = 1, VkImageCreateFlags flags = 0);

		/* VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL for real update of an existing texture */
		bool updateTextureImage(VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t texWidth, uint32_t texHeight,
			VkFormat texFormat, uint32_t layerCount, const void* imageData, VkImageLayout sourceImageLayout = VK_IMAGE_LAYOUT_UNDEFINED);

		VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		VkFormat findDepthFormat();

		bool createUniformBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize bufferSize);

		bool createSharedBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

		bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

		VulkanBuffer addBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, bool createMapping = false);

		inline VulkanBuffer addUniformBuffer(VkDeviceSize bufferSize, bool createMapping = false)
		{
			return addBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, createMapping); /* for debugging we make it host-visible */
		}

		inline VulkanBuffer addIndirectBuffer(VkDeviceSize bufferSize, bool createMapping = false) {
			return addBuffer(bufferSize, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, // | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, createMapping); /* for debugging we make it host-visible */
		}

		inline VulkanBuffer addStorageBuffer(VkDeviceSize bufferSize, bool createMapping = false)
		{
			return addBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, createMapping); /* for debugging we make it host-visible */
		}

		inline VulkanBuffer addLocalDeviceStorageBuffer(VkDeviceSize bufferSize, bool createMapping = false)
		{
			return addBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, createMapping); /* for debugging we make it host-visible */
		}

		/* Allocate and upload vertex & index buffer pair */
		VulkanBuffer addVertexBuffer(uint32_t indexBufferSize, const void* indexData, uint32_t vertexBufferSize, const void* vertexData);

		size_t allocateVertexBuffer(VkBuffer* storageBuffer, VkDeviceMemory* storageBufferMemory, size_t vertexDataSize, const void* vertexData, size_t indexDataSize, const void* indexData);

		/** Copy [data] to GPU device buffer */
		void uploadBufferData(const VkDeviceMemory& bufferMemory, VkDeviceSize deviceOffset, const void* data, const size_t dataSize);

		/** Copy GPU device buffer data to [outData] */
		void downloadBufferData(const VkDeviceMemory& bufferMemory, VkDeviceSize deviceOffset, void* outData, size_t dataSize);

		inline uint32_t getVulkanBufferAlignment()
		{
			VkPhysicalDeviceProperties devProps;
			vkGetPhysicalDeviceProperties(m_Context.physicalDevice, &devProps);
			return static_cast<uint32_t>(devProps.limits.minStorageBufferOffsetAlignment);
		}

		bool createColorOnlyRenderPass(VkRenderPass* renderPass, const RenderPassCreateInfo& ci, VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM);
		bool createColorAndDepthRenderPass(bool useDepth, VkRenderPass* renderPass, const RenderPassCreateInfo& ci, VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM);
		bool createDepthOnlyRenderPass(VkRenderPass* renderPass, const RenderPassCreateInfo& ci);

		RenderPass addFullScreenPass(bool useDepth = true, const RenderPassCreateInfo ci = RenderPassCreateInfo());

		RenderPass addRenderPass(const std::vector<VulkanTexture>& outputs, const RenderPassCreateInfo& ci = {
			true, true, eRenderPassBit_Offscreen | eRenderPassBit_First }, bool useDepth = true);

		RenderPass addDepthRenderPass(const std::vector<VulkanTexture>& outputs, const RenderPassCreateInfo ci = {
			false, true, eRenderPassBit_Offscreen | eRenderPassBit_First });

		bool createPipelineLayout(VkDescriptorSetLayout dsLayout, VkPipelineLayout* pipelineLayout);

		bool createPipelineLayoutWithConstants(VkDescriptorSetLayout dsLayout, VkPipelineLayout* pipelineLayout, uint32_t vtxConstSize, uint32_t fragConstSize);

		VkPipelineLayout addPipelineLayout(VkDescriptorSetLayout dsLayout, uint32_t vtxConstSize = 0, uint32_t fragConstSize = 0);

		VkPipeline addPipeline(VkRenderPass renderPass, VkPipelineLayout pipelineLayout,
			const std::vector<const char*>& shaderFiles,
			const PipelineInfo& pipelineParams = PipelineInfo{
			0, 0, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			true, false, false });

		VkResult createComputePipeline(VkShaderModule computeShader, VkPipelineLayout pipelineLayout, VkPipeline* pipeline);

		/* Calculate the descriptor pool size from the list of buffers and textures */
		VkDescriptorPool addDescriptorPool(const DescriptorSetInfo& dsInfo, uint32_t dSetCount = 1);

		VkDescriptorSetLayout addDescriptorSetLayout(const DescriptorSetInfo& dsInfo);

		VkDescriptorSet addDescriptorSet(VkDescriptorPool descriptorPool, VkDescriptorSetLayout dsLayout);

		void updateDescriptorSet(VkDescriptorSet ds, const DescriptorSetInfo& dsInfo);

		bool createColorAndDepthFramebuffers(VkRenderPass renderPass, VkImageView depthImageView, std::vector<VkFramebuffer>& swapchainFramebuffers);

		VkFramebuffer addFramebuffer(RenderPass renderPass, const std::vector<VulkanTexture>& images);

		std::vector<VkFramebuffer> addFramebuffers(VkRenderPass renderPass, VkImageView depthView = VK_NULL_HANDLE);

		/**  Helper functions for small Chapter 8/9 demos */
		std::pair<BufferAttachment, BufferAttachment> makeMeshBuffers(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);

		std::pair<BufferAttachment, BufferAttachment> loadMeshToBuffer(const char* filename, bool useTextureCoordinates, bool useNormals,
			std::vector<float>& vertices,
			std::vector<unsigned int>& indices);

		std::pair<BufferAttachment, BufferAttachment> createPlaneBuffer_XZ(float sx, float sz);
		std::pair<BufferAttachment, BufferAttachment> createPlaneBuffer_XY(float sx, float sy);

		VkResult createShaderModule(ShaderModule* shader, const char* fileName);

	private:
		VulkanContext m_Context;
		DeviceDesc m_DeviceDesc;
		VulkanResources m_Resources;
		CommandList m_CommandList;

		// array of submission queues
		std::array<std::unique_ptr<Queue>, uint32_t(CommandQueue::Count)> m_Queues;

		// a list of all queues indices (for shared buffer allocations)
		std::vector<uint32_t> m_DeviceQueueIndices;

		bool createGraphicsPipeline(
			VkRenderPass renderPass, VkPipelineLayout pipelineLayout,
			const std::vector<const char*>& shaderFiles,
			VkPipeline* pipeline,
			VkPrimitiveTopology topology,
			bool useDepth,
			bool useBlending,
			bool dynamicScissorState,
			int32_t customWidth,
			int32_t customHeight,
			uint32_t numPatchControlPoints);
	};

	class CommandList final : public IRHICommandList
	{
	public:
		CommandList(VulkanContext& ctx);
		virtual ~CommandList();

		VkCommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(VkCommandBuffer commandBuffer);
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount = 1, uint32_t mipLevels = 1);
		void transitionImageLayoutCmd(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount = 1, uint32_t mipLevels = 1);

		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount = 1);
		void copyMIPBufferToImage(VkBuffer buffer, VkImage image, uint32_t mipLevels, uint32_t width, uint32_t height, uint32_t bytesPP, uint32_t layerCount = 1);
		void copyImageToBuffer(VkImage image, VkBuffer buffer, uint32_t width, uint32_t height, uint32_t layerCount = 1);

		bool draw(const std::function<void(uint32_t)>& updateBuffersFunc, const std::function<void(VkCommandBuffer, uint32_t)>& composeFrameFunc);

	private:
		Device* m_Device;
		VulkanContext m_Context;
	};
}
