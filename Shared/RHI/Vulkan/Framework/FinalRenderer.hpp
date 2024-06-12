#pragma once

#include "RHI/Vulkan/Framework/MultiRenderer.hpp"

#include "RHI/Vulkan/Framework/Effects/LuminanceCalculator.hpp"

#include <algorithm>
#include <numeric>

const uint32_t ShadowSize = 8192;

/**
	The "finalized" variant of MultiRenderer

	For Chapter 10's demo we need to extract indirect buffer with rendering commands
	and expose it externally so that different parts of the same scene can be rendered
	(in our case - transparent and opaque objects)

	This is almost a line-by-line repetition of MultiRenderer class from vkFramework,
	but the additional 'objectIndices' parameter shows which scene items are rendered here
*/
struct BaseMultiRenderer : public Renderer
{
	BaseMultiRenderer(
		VulkanRenderContext& ctx,
		VKSceneData& sceneData,
		// indices of objects from sceneData's shape list
		const std::vector<int>& objectIndices,
		const char* vtxShaderFile = DefaultMeshVertexShader,
		const char* fragShaderFile = DefaultMeshFragmentShader,
		const std::vector<VulkanTexture>& outputs = std::vector<VulkanTexture>{},
		RenderPass screenRenderPass = RenderPass(),
		const std::vector<BufferAttachment>& auxBuffers = std::vector<BufferAttachment>{},
		const std::vector<TextureAttachment>& auxTextures = std::vector<TextureAttachment>{});

	void updateIndirectBuffers(size_t currentImage, bool* visibility = nullptr);
	void updateIndirectBuffersOptimized(VkDeviceMemory* indirectTransferMemory, bool* visibility = nullptr);

	void fillCommandBuffer(VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE) override;

	void updateBuffers(size_t currentImage) override {
		updateUniformBuffer((uint32_t)currentImage, 0, sizeof(ubo_), &ubo_);
	}

	inline void setMatrices(const glm::mat4& proj, const glm::mat4& view) {
		const glm::mat4 m1 = glm::scale(glm::mat4(1.f), glm::vec3(1.f, -1.f, 1.f));
		ubo_.proj_ = proj;
		ubo_.view_ = view * m1;
	}

	inline void setCameraPosition(const glm::vec3& cameraPos) { ubo_.cameraPos_ = glm::vec4(cameraPos, 1.0f); }

	inline const VKSceneData& getSceneData() const { return sceneData_; }

private:
	VKSceneData& sceneData_;

	std::vector<int> indices_;

	std::vector<VulkanBuffer> indirect_;
	std::vector<VulkanBuffer> shape_;

	struct UBO {
		mat4 proj_;
		mat4 view_;
		vec4 cameraPos_;
	} ubo_;
};

// Extract a list of indices of opaque objects
inline std::vector<int> getOpaqueIndices(const VKSceneData& sd)
{
	std::vector<int> src_indices(sd.shapes_.size());
	std::iota(src_indices.begin(), src_indices.end(), 0);

	std::vector<int> list;

	std::copy_if(src_indices.begin(), src_indices.end(), std::back_inserter(list),
		[&sd](const auto& idx)
		{
			const auto c = sd.shapes_[idx];        // DrawData;
			const auto& mtl = sd.materials_[c.materialIndex];
			return (mtl.flags_ & sMaterialFlags_Transparent) == 0;
		});

	return list;
}

// Extract a list of transparent objects
inline std::vector<int> getTransparentIndices(const VKSceneData& sd)
{
	std::vector<int> src_indices(sd.shapes_.size());
	std::iota(src_indices.begin(), src_indices.end(), 0);

	std::vector<int> list;

	std::copy_if(src_indices.begin(), src_indices.end(), std::back_inserter(list),
		[&sd](const auto& idx)
		{
			const auto c = sd.shapes_[idx];        // DrawData;
			const auto& mtl = sd.materials_[c.materialIndex];
			return (mtl.flags_ & sMaterialFlags_Transparent) > 0;
		});

	return list;
}

struct LightParamsBuffer
{
	mat4 proj;
	mat4 view;

	uint32_t width;
	uint32_t height;
};

// Single item in the OIT buffer. See Chapter 10's GL03_OIT demo and "Order-independent Transparency" Recipe in the book
struct TransparentFragment {
	vec4 color;
	float depth;
	uint32_t next;
};

/**
	This the final variant of the scene rendering class
	It manages lists of opaque/transparent objects and uses two BaseMultiRenderer instances
	The transparent objects renderer fills the auxilliary OIT linked list buffer (see VK02_Glass.frag shader)
	and clears this per-pixel transparent fragment linked list at each frame
	OIT buffer composition is also performed by this class

	Additionally, this class provides boolean flags to enable/disable shadows and transparent objects

	Technically, this can be implemented as a CompositeRenderer
	and OIT can be extracted to a separate class,
	but it also would require a few more barrier classes, so we opted for a straight-forward solution
	similar to a list of OpenGL commands
*/

struct FinalMultiRenderer : public Renderer
{
	FinalMultiRenderer(
		VulkanRenderContext& ctx,
		VKSceneData& sceneData,
		const std::vector<VulkanTexture>& outputs = std::vector<VulkanTexture>{});

	void fillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE) override;

	void updateBuffers(size_t currentImage) override;

	void updateIndirectBuffers(size_t currentImage, bool* visibility = nullptr);

	inline void setMatrices(const glm::mat4& proj, const glm::mat4& view) {
		transparentRenderer.setMatrices(proj, view);
		opaqueRenderer.setMatrices(proj, view);
	}

	inline void setLightParameters(const glm::mat4& lightProj, const glm::mat4& lightView)
	{
		const LightParamsBuffer lightParamsBuffer{ lightProj, lightView, ctx_.vkDev.framebufferWidth, ctx_.vkDev.framebufferHeight };

		uploadBufferData(ctx_.vkDev, lightParams.memory, 0, &lightParamsBuffer, sizeof(LightParamsBuffer));

		shadowRenderer.setMatrices(lightProj, lightView);
	}

	inline void setCameraPosition(const glm::vec3& cameraPos) {
		transparentRenderer.setCameraPosition(cameraPos);
		opaqueRenderer.setCameraPosition(cameraPos);
		shadowRenderer.setCameraPosition(cameraPos);
	}

	inline const VKSceneData& getSceneData() const { return sceneData_; }

	bool checkLoadedTextures();

	VulkanTexture shadowColor;
	VulkanTexture shadowDepth;

	VulkanBuffer lightParams;

	VulkanBuffer atomicBuffer;
	VulkanBuffer headsBuffer;
	VulkanBuffer oitBuffer;

	VulkanTexture outputColor;

	bool enableShadows = true;
	bool renderTransparentObjects = true;

private:
	VKSceneData& sceneData_;

	BaseMultiRenderer transparentRenderer;
	BaseMultiRenderer opaqueRenderer;

	BaseMultiRenderer shadowRenderer;

	ShaderOptimalToColorBarrier colorToAttachment;
	ShaderOptimalToDepthBarrier depthToAttachment;

	VulkanBuffer whBuffer;

	QuadProcessor clearOIT;
	QuadProcessor composeOIT;

	struct UBO {
		uint32_t width;
		uint32_t height;
	} ubo_;

	ShaderOptimalToColorBarrier outputToAttachment;
	ColorToShaderOptimalBarrier outputToShader;
};
