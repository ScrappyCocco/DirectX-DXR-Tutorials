#pragma once

#include <d3d12.h>
#include "Source/Dx12/InterfacePointers.h"

namespace DirectXUtil
{
	namespace Structs
	{
		struct FrameObject
		{
			SampleFramework::ID3D12CommandAllocatorPtr pCmdAllocator;
			SampleFramework::ID3D12ResourcePtr pSwapChainBuffer;
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
		};
	}
}
