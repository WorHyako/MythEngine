#include <RHICommon.hpp>

namespace RHI
{
	bool IDynamicRHI::CreateDevice(DeviceParams& params)
	{
		m_DeviceParams = params;

		CreateDevice();

		return true;
	}

	void IDynamicRHI::BackBufferResized()
	{
		uint32_t backBufferCount = GetBackBufferCount();
		m_SwapChainFramebuffers.resize(backBufferCount);
		for(uint32_t index = 0; index < backBufferCount; index++)
		{
			RenderPassCreateInfo ci{};
			ci.clearColor = true;
			ci.clearDepth = true;
			ci.useDepth = false;
			ci.flags = eRenderPassBit_First | eRenderPassBit_Last;
			ci.format = Format::BGRA8_UNORM;
			ci.numOutputs = 1;
			IRenderPass* renderPass = getDevice()->createRenderPass(ci);
			m_SwapChainFramebuffers[index] = getDevice()->createFramebuffer(renderPass, { GetBackBuffer(index) });
		}
	}

}
