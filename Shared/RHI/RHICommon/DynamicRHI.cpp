#include <RHICommon.hpp>

namespace RHI
{
	bool IDynamicRHI::CreateDevice(DeviceParams& params)
	{
		m_DeviceParams = params;

		CreateDevice();
	}

	void IDynamicRHI::BackBufferResized()
	{
		uint32_t backBufferCount = GetBackBufferCount();
		m_SwapChainFramebuffers.resize(backBufferCount);
		for(uint32_t index = 0; index < backBufferCount; index++)
		{
			IRenderPass* renderPass = getDevice()->createRenderPass();
			m_SwapChainFramebuffers[index] = getDevice()->createFramebuffer(renderPass, );
		}
	}

}
