#pragma once
#include <RHI/RHICommon/RHICommon.hpp>

namespace mythSystem
{
	class RendererInterface
	{
	public:
		RendererInterface(RHI::IDevice* device)
			: m_Device(device)
		{

		}
		virtual ~RendererInterface() {};

		virtual void renderScene() = 0;
		virtual void initializeRender() = 0;

	protected:
		RHI::IDevice* m_Device;
		RHI::IRHICommandList* m_CommandList;
	};
}