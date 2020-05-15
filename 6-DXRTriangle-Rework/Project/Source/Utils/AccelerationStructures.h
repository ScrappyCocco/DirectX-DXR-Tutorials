#pragma once

#include "../Dx12/InterfacePointers.h"
#include "Structs/AccelerationStructureBuffers.h"
#include <cstdint>
#include <d3d12.h>

namespace DirectXUtil
{
	class AccelerationStructures
	{
	public:
		static const D3D12_HEAP_PROPERTIES kUploadHeapProps;
		static const D3D12_HEAP_PROPERTIES kDefaultHeapProps;
		
		static SampleFramework::ID3D12ResourcePtr createBuffer(
			SampleFramework::ID3D12Device5Ptr pDevice,
			uint64_t size,
			D3D12_RESOURCE_FLAGS flags,
			D3D12_RESOURCE_STATES initState,
			const D3D12_HEAP_PROPERTIES& heapProps
		);

		static SampleFramework::ID3D12ResourcePtr createTriangleVB(SampleFramework::ID3D12Device5Ptr pDevice);

		static SampleFramework::ID3D12ResourcePtr createPlaneVB(SampleFramework::ID3D12Device5Ptr pDevice);

		static DirectXUtil::Structs::AccelerationStructureBuffers createBottomLevelAS(
			SampleFramework::ID3D12Device5Ptr pDevice,
			SampleFramework::ID3D12GraphicsCommandList4Ptr pCmdList,
			SampleFramework::ID3D12ResourcePtr pVB[],
			const uint32_t vertexCount[],
			uint32_t geometryCount
		);
		
		static void buildTopLevelAS(
			SampleFramework::ID3D12Device5Ptr pDevice,
			SampleFramework::ID3D12GraphicsCommandList4Ptr pCmdList,
			SampleFramework::ID3D12ResourcePtr pBottomLevelAS[2],
			uint64_t& tlasSize,
			float rotation,
			bool update,
			DirectXUtil::Structs::AccelerationStructureBuffers& buffers
		);
	};
}
