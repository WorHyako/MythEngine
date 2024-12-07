#include <RHICommon.hpp>

namespace RHI
{
	bool IDynamicRHI::CreateDevice(DeviceParams& params)
	{
		m_DeviceParams = params;

		if(gra)
		CreateDevice();
	}
}
