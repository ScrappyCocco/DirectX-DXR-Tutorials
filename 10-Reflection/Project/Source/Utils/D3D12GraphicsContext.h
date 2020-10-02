#pragma once

#include <cstdint>
#include "Structs/AccelerationStructureBuffers.h"

namespace DirectXUtil
{
	class D3D12GraphicsContext
	{
	public:
		static uint32_t getDefaultSwapChainBuffers();

		SampleFramework::IDXGISwapChain3Ptr createDxgiSwapChain(
			SampleFramework::IDXGIFactory4Ptr pFactory,
			HWND hwnd,
			uint32_t width,
			uint32_t height,
			DXGI_FORMAT format,
			SampleFramework::ID3D12CommandQueuePtr pCommandQueue
		) const;

		static void GetHardwareAdapter(
			_In_ IDXGIFactory2* pFactory,
			_Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter
		);

		SampleFramework::ID3D12Device5Ptr createDevice(
			SampleFramework::IDXGIFactory4Ptr pDxgiFactory,
			bool m_useWarpDevice = false
		) const;

		SampleFramework::ID3D12CommandQueuePtr createCommandQueue(SampleFramework::ID3D12Device5Ptr pDevice) const;

		SampleFramework::ID3D12DescriptorHeapPtr createDescriptorHeap(
			SampleFramework::ID3D12Device5Ptr pDevice,
			uint32_t count,
			D3D12_DESCRIPTOR_HEAP_TYPE type,
			bool shaderVisible
		) const;

		static D3D12_CPU_DESCRIPTOR_HANDLE createRTV(
			SampleFramework::ID3D12Device5Ptr pDevice,
			SampleFramework::ID3D12ResourcePtr pResource,
			SampleFramework::ID3D12DescriptorHeapPtr pHeap,
			uint32_t& usedHeapEntries,
			DXGI_FORMAT format
		);

		static void resourceBarrier(
			SampleFramework::ID3D12GraphicsCommandList4Ptr pCmdList,
			SampleFramework::ID3D12ResourcePtr pResource,
			D3D12_RESOURCE_STATES stateBefore,
			D3D12_RESOURCE_STATES stateAfter
		);

		static uint64_t submitCommandList(
			SampleFramework::ID3D12GraphicsCommandList4Ptr pCmdList,
			SampleFramework::ID3D12CommandQueuePtr pCmdQueue,
			SampleFramework::ID3D12FencePtr pFence,
			uint64_t fenceValue
		);
	private:
		static const uint32_t kDefaultSwapChainBuffers = 3;
	};
}
