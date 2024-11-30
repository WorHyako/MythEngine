#pragma once
#include <RHI/RHICommon/RHICommon.hpp>

namespace mythSystem
{
	class RendererInterface
	{
	public:
		RendererInterface() {};
		virtual ~RendererInterface() {};

		virtual void renderScene() = 0;
		virtual void initializeRender() = 0;

	protected:
		RHI::IDevice* m_Device;
	};
}