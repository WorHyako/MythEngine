#pragma once
#include <RHICommon.hpp>

namespace mythSystem
{
	class RendererInterface
	{
	public:
		RendererInterface(RHI::IDynamicRHI* dynamicRHI)
			: m_DynamicRHI(dynamicRHI)
			, m_Device(dynamicRHI->getDevice())
		{

		}
		virtual ~RendererInterface() {};

		virtual void updateBuffers() = 0;
		virtual void composeFrame() = 0;
		virtual bool renderScene() = 0;
		virtual bool initializeRender() = 0;

	protected:
		RHI::IDynamicRHI* m_DynamicRHI;
		RHI::DeviceHandle m_Device;
		std::vector<RHI::IRHICommandList*> m_CommandLists;
	};
}