#pragma once

#include <RHI/RHICommon/Common/Resources.hpp>

#include <cstdint>
#include <vector>

namespace RHI
{
	class ITexture;
	class IDevice;
    class IFramebuffer;

	enum class GraphicsAPI : uint8_t
    {
        OGL,
        D3D12,
        VULKAN
    };

    enum class Format : uint8_t
    {
        UNKNOWN,

        R8_UINT,
        R8_SINT,
        R8_UNORM,
        R8_SNORM,
        RG8_UINT,
        RG8_SINT,
        RG8_UNORM,
        RG8_SNORM,
        R16_UINT,
        R16_SINT,
        R16_UNORM,
        R16_SNORM,
        R16_FLOAT,
        BGRA4_UNORM,
        B5G6R5_UNORM,
        B5G5R5A1_UNORM,
        RGBA8_UINT,
        RGBA8_SINT,
        RGBA8_UNORM,
        RGBA8_SNORM,
        BGRA8_UNORM,
        SRGBA8_UNORM,
        SBGRA8_UNORM,
        R10G10B10A2_UNORM,
        R11G11B10_FLOAT,
        RG16_UINT,
        RG16_SINT,
        RG16_UNORM,
        RG16_SNORM,
        RG16_FLOAT,
        R32_UINT,
        R32_SINT,
        R32_FLOAT,
        RGBA16_UINT,
        RGBA16_SINT,
        RGBA16_FLOAT,
        RGBA16_UNORM,
        RGBA16_SNORM,
        RG32_UINT,
        RG32_SINT,
        RG32_FLOAT,
        RGB32_UINT,
        RGB32_SINT,
        RGB32_FLOAT,
        RGBA32_UINT,
        RGBA32_SINT,
        RGBA32_FLOAT,

        D16,
        D24S8,
        X24G8_UINT,
        D32,
        D32S8,
        X32G8_UINT,

        BC1_UNORM,
        BC1_UNORM_SRGB,
        BC2_UNORM,
        BC2_UNORM_SRGB,
        BC3_UNORM,
        BC3_UNORM_SRGB,
        BC4_UNORM,
        BC4_SNORM,
        BC5_UNORM,
        BC5_SNORM,
        BC6H_UFLOAT,
        BC6H_SFLOAT,
        BC7_UNORM,
        BC7_UNORM_SRGB,

        COUNT,
    };

    enum class CommandQueue : uint8_t
    {
        Graphics = 0,
        Compute,
        Copy,

        Count
    };

    struct CommandListParameters
    {
    	// Type of the queue that this command list is to be executed on.
        // COPY and COMPUTE queues have limited subsets of methods available.
        CommandQueue queueType = CommandQueue::Graphics;
    };

    struct DrawArguments
    {
        uint32_t vertexCount = 0;
        uint32_t instanceCount = 1;
        uint32_t startIndexLocation = 0;
        uint32_t startVertexLocation = 0;
        uint32_t startInstanceLocation = 0;

        DrawArguments& setVertexCount(uint32_t value) { vertexCount = value; return *this; }
        DrawArguments& setInstanceCount(uint32_t value) { instanceCount = value; return *this; }
        DrawArguments& setStartIndexLocation(uint32_t value) { startIndexLocation = value; return *this; }
        DrawArguments& setStartVertexLocation(uint32_t value) { startVertexLocation = value; return *this; }
        DrawArguments& setStartInstanceLocation(uint32_t value) { startInstanceLocation = value; return *this; }
    };

    class IRHICommandList : public IResource
    {
    public:
        virtual void beginSingleTimeCommands() = 0;
        virtual void endSingleTimeCommands() = 0;
        virtual void draw(const DrawArguments& args) = 0;
    };

    class IInstance : public IResource
    {
	    
    };

    struct Resolution
    {
        uint32_t width = 0;
        uint32_t height = 0;
    };

    struct DeviceParams
    {
        bool useGraphicsQueue = true;
        bool useComputeQueue = false;
        bool useTransferQueue = false;

        bool enableDebugRuntime = false;

        uint32_t backBufferWidth = -1;
        uint32_t backBufferHeight = -1;
        uint32_t maxFramesInFlight = 2;

        bool supportScreenshots = false;
    };

    class IDynamicRHI
    {
    public:
        IDynamicRHI() = default;
        virtual ~IDynamicRHI() = default;

        bool CreateDevice(DeviceParams& params);
        virtual void CreateDevice() = 0;
        virtual bool BeginFrame() = 0;
        virtual bool Present() = 0;
        virtual void BackBufferResized();
        virtual ITexture* GetBackBuffer(uint32_t index) = 0;
        virtual uint32_t GetBackBufferCount() = 0;
        virtual IFramebuffer* GetFramebuffer(uint32_t index) = 0;
        virtual IDevice* getDevice() const = 0;
        virtual GraphicsAPI getGraphicsAPI() const = 0;

    protected:
        DeviceParams m_DeviceParams;
        std::vector<IFramebuffer*> m_SwapChainFramebuffers;
    };

    class IRHIModule
    {
    public:
        IRHIModule() = default;
        virtual ~IRHIModule() = default;

    public:
        virtual IDynamicRHI* createRHI() = 0;
        virtual void* getWindowInterface() = 0;
    };

    struct TextureDesc
    {
        uint32_t width = 1;
        uint32_t height = 1;
        uint32_t depth = 1;
    };

    class ITexture : public IResource
    {
    public:
        uint32_t width;
        uint32_t height;
        uint32_t depth;
    };

    class IImage : public IResource
    {
	    
    };

    struct BufferDesc
    {
	    
    };

    class IBuffer : public IResource
    {

    };

    struct ShaderDesc
    {
	    
    };

    class IShader : public IResource
    {
	    
    };

    struct FramebufferDesc
    {
	    
    };

    enum eRenderPassBit : uint8_t
    {
        eRenderPassBit_First = 0x01, // clear the attachment
        eRenderPassBit_Last = 0x02, // transition to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        eRenderPassBit_Offscreen = 0x04, // transition to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        eRenderPassBit_OffscreenInternal = 0x08, // keepVK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
    };

    struct RenderPassCreateInfo final
    {
        bool clearColor = false;
        bool clearDepth = false;
        bool useDepth = false;
        uint32_t numOutputs = 0;
        Format format = Format::UNKNOWN;
        uint8_t flags = 0;
    };

    class IRenderPass : public IResource
    {
	    
    };

    class IFramebuffer : public IResource
    {
    private:
        IRenderPass* m_RenderPass;
    };

    enum class PrimitiveType : uint8_t
    {
        PointList,
        LineList,
        TriangleList,
        TriangleStrip,
        TriangleFan,
        TriangleListWithAdjacency,
        TriangleStripWithAdjacency,
        PatchList
    };

    /* A structure with pipeline parameters */
    struct GraphicsPipelineInfo
    {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t topology = 3; /* defaults to triangles*/

        bool useDepth = true;
        bool useBlending = false;
        bool dynamicScissorState = false;

        uint32_t patchControlPoints = 0;
    };

    struct GraphicsPipelineDesc
    {
        PrimitiveType primType = PrimitiveType::TriangleList;

        IShader* VS;
        IShader* HS;
        IShader* DS;
        IShader* GS;
        IShader* PS;

        //VkPipelineLayout pipelineLayout,
        GraphicsPipelineInfo pipelineInfo;

        GraphicsPipelineDesc& setPrimType(PrimitiveType value) { primType = value; return *this; }
        GraphicsPipelineDesc& setVertexShader(IShader* value) { VS = value; return *this; }
        GraphicsPipelineDesc& setTessallationControlShader(IShader* value) { HS = value; return *this; }
        GraphicsPipelineDesc& setTessallationEvaluationShader(IShader* value) { DS = value; return *this; }
        GraphicsPipelineDesc& setGeometryShader(IShader* value) { GS = value; return *this; }
        GraphicsPipelineDesc& setPixelShader(IShader* value) { PS = value; return *this; }
    };

    class IGraphicsPipeline : public IResource
    {
        virtual const GraphicsPipelineDesc& getDesc() const = 0;
    };

    class IDevice : public IResource
    {
    public:
        virtual IRHICommandList* createCommandList(const CommandListParameters& params = CommandListParameters()) = 0;
        virtual uint64_t executeCommandLists(std::vector<IRHICommandList*>& commandLists, size_t numCommandLists, CommandQueue executionQueue = CommandQueue::Graphics) = 0;
        virtual GraphicsAPI getGraphicsAPI() const = 0;
        virtual IRenderPass* createRenderPass(const RenderPassCreateInfo& ci = RenderPassCreateInfo()) = 0;
        virtual IFramebuffer* createFramebuffer(IRenderPass* renderPass, const std::vector<ITexture*>& images) = 0;
        virtual IGraphicsPipeline* createGraphicsPipeline(const GraphicsPipelineDesc& desc, IFramebuffer* framebuffer) = 0;
    };
}
