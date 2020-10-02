#pragma once

#include "../Dx12/InterfacePointers.h"
#include "Structs/AccelerationStructureBuffers.h"
#include <cstdint>
#include <d3d12.h>
#include <vector>

namespace DirectXUtil
{
	class AccelerationStructures
	{
	public:
		static const D3D12_HEAP_PROPERTIES kUploadHeapProps;
		static const D3D12_HEAP_PROPERTIES kDefaultHeapProps;

		enum PrimitiveType
		{
			SPHERE,
			CUBE,
			QUAD
		};

		struct ShapeResources
		{
			//Vertex
			SampleFramework::ID3D12ResourcePtr vertexBuffer;
			unsigned int vertexCount;
			//Index
			SampleFramework::ID3D12ResourcePtr indexBuffer;
			unsigned int indexCount;
		};

		static SampleFramework::ID3D12ResourcePtr createBuffer(
			SampleFramework::ID3D12Device5Ptr pDevice,
			uint64_t size,
			D3D12_RESOURCE_FLAGS flags,
			D3D12_RESOURCE_STATES initState,
			const D3D12_HEAP_PROPERTIES& heapProps
		);

		static ShapeResources* createdPrimitive;

		static ShapeResources* createPrimitive(SampleFramework::ID3D12Device5Ptr pDevice, PrimitiveType type);

		static DirectXUtil::Structs::AccelerationStructureBuffers createPlaneBottomLevelAS(
			SampleFramework::ID3D12Device5Ptr pDevice,
			SampleFramework::ID3D12GraphicsCommandList4Ptr pCmdList
		);

		static DirectXUtil::Structs::AccelerationStructureBuffers createPrimitiveBottomLevelAS(
			SampleFramework::ID3D12Device5Ptr pDevice,
			SampleFramework::ID3D12GraphicsCommandList4Ptr pCmdList
		);

		static void buildTopLevelAS(
			SampleFramework::ID3D12Device5Ptr pDevice,
			SampleFramework::ID3D12GraphicsCommandList4Ptr pCmdList,
			std::vector<Structs::AccelerationStructureBuffers> pBottomLevelAS,
			uint64_t& tlasSize,
			DirectXUtil::Structs::AccelerationStructureBuffers& buffers
		);

	private:
		static DirectXUtil::Structs::AccelerationStructureBuffers createBottomLevelAS(
			SampleFramework::ID3D12Device5Ptr pDevice,
			SampleFramework::ID3D12GraphicsCommandList4Ptr pCmdList,
			ShapeResources* shape
		);
	};
}
