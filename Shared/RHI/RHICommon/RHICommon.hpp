#pragma once

#include <RHI/RHICommon/Common/Resources.hpp>

#include <cstdint>
#include <vector>

namespace RHI
{
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

    class IRHICommandList : public IResource
    {
        virtual void draw() = 0;
    };

    class IInstance : public IResource
    {
	    
    };

    class IDevice : public IResource
    {
    public:
        virtual IRHICommandList* createCommandList(const CommandListParameters& params = CommandListParameters()) = 0;
        virtual uint64_t executeCommandList(std::vector<IRHICommandList*>& commandLists, size_t numCommandLists, CommandQueue executionQueue = CommandQueue::Graphics) = 0;
        virtual GraphicsAPI getGraphicsAPI() const = 0;
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
        uint32_t maxFramesInFlight = -1;

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
        virtual IDevice* getDevice() const = 0;
        virtual GraphicsAPI getGraphicsAPI() const = 0;

    protected:
        DeviceParams m_DeviceParams;
    };

    class IRHIModule
    {
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

    class IFramebuffer : public IResource
    {
	    
    };
}
