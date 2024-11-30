#pragma once
#include <RHI/RHICommon/RHICommon.hpp>

namespace mythSystem
{
	class RendererInterface
	{
	public:
		RendererInterface() {};
		virtual ~RendererInterface() {};

		virtual void RenderScene() = 0;
		virtual void InitializeRender() = 0;

	protected:
		RHI::IDevice* device_;
	};
}