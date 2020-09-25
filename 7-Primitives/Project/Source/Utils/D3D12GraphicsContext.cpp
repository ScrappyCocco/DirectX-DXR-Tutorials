#include "D3D12GraphicsContext.h"

#include "Source/DXSampleHelper.h"

uint32_t DirectXUtil::D3D12GraphicsContext::getDefaultSwapChainBuffers()
{
	return kDefaultSwapChainBuffers;
}

SampleFramework::IDXGISwapChain3Ptr DirectXUtil::D3D12GraphicsContext::createDxgiSwapChain(
	SampleFramework::IDXGIFactory4Ptr pFactory, const HWND hwnd, const uint32_t width, const uint32_t height,
	const DXGI_FORMAT format,
	SampleFramework::ID3D12CommandQueuePtr pCommandQueue) const
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = kDefaultSwapChainBuffers;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = format;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	// CreateSwapChainForHwnd() doesn't accept IDXGISwapChain3
	SampleFramework::IDXGISwapChain1Ptr pSwapChain;

	const HRESULT hr = pFactory->CreateSwapChainForHwnd(
		pCommandQueue.Get(),
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&pSwapChain
	);

	ThrowIfFailed(hr, "Failed to create the swap-chain");

	SampleFramework::IDXGISwapChain3Ptr pSwapChain3;
	ThrowIfFailed(pSwapChain->QueryInterface(IID_PPV_ARGS(&pSwapChain3)), "Failed to QueryInterface on swap-chain");
	return pSwapChain3;
}

void DirectXUtil::D3D12GraphicsContext::GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
	*ppAdapter = nullptr;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			// If you want a software adapter, pass in "/warp" on the command line.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the
		// actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}

	*ppAdapter = adapter.Detach();
}

SampleFramework::ID3D12Device5Ptr DirectXUtil::D3D12GraphicsContext::createDevice(
	SampleFramework::IDXGIFactory4Ptr pDxgiFactory, const bool m_useWarpDevice) const
{
	SampleFramework::ID3D12Device5Ptr pDevice;

	//Create Device
	if (m_useWarpDevice)
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(pDxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(),
		                                D3D_FEATURE_LEVEL_12_1,
		                                IID_PPV_ARGS(&pDevice)));
	}
	else
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(pDxgiFactory.Get(), &hardwareAdapter);

		ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(),
		                                D3D_FEATURE_LEVEL_12_1,
		                                IID_PPV_ARGS(&pDevice)));
	}

	//Check Raytracing Support
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
	ThrowIfFailed(pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,
	                                           &options5, sizeof(options5)));
	if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
	{
		ThrowError("Raytracing is not supported on this device. Make sure your GPU supports DXR");
		exit(1);
	}

	return pDevice;
}

SampleFramework::ID3D12CommandQueuePtr DirectXUtil::D3D12GraphicsContext::createCommandQueue(
	SampleFramework::ID3D12Device5Ptr pDevice) const
{
	SampleFramework::ID3D12CommandQueuePtr pQueue;
	D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed(pDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&pQueue)));
	return pQueue;
}

SampleFramework::ID3D12DescriptorHeapPtr DirectXUtil::D3D12GraphicsContext::createDescriptorHeap(
	SampleFramework::ID3D12Device5Ptr pDevice, const uint32_t count, const D3D12_DESCRIPTOR_HEAP_TYPE type,
	const bool shaderVisible) const
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = count;
	desc.Type = type;
	desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	SampleFramework::ID3D12DescriptorHeapPtr pHeap;
	ThrowIfFailed(pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pHeap)));
	return pHeap;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXUtil::D3D12GraphicsContext::createRTV(SampleFramework::ID3D12Device5Ptr pDevice,
                                                                         SampleFramework::ID3D12ResourcePtr pResource,
                                                                         SampleFramework::ID3D12DescriptorHeapPtr pHeap,
                                                                         uint32_t& usedHeapEntries,
                                                                         const DXGI_FORMAT format)
{
	D3D12_RENDER_TARGET_VIEW_DESC desc = {};
	desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	desc.Format = format;
	desc.Texture2D.MipSlice = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += usedHeapEntries * pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	usedHeapEntries++;
	pDevice->CreateRenderTargetView(pResource.Get(), &desc, rtvHandle);
	return rtvHandle;
}

void DirectXUtil::D3D12GraphicsContext::resourceBarrier(SampleFramework::ID3D12GraphicsCommandList4Ptr pCmdList,
                                                        SampleFramework::ID3D12ResourcePtr pResource,
                                                        const D3D12_RESOURCE_STATES stateBefore,
                                                        const D3D12_RESOURCE_STATES stateAfter)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = pResource.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = stateBefore;
	barrier.Transition.StateAfter = stateAfter;
	pCmdList->ResourceBarrier(1, &barrier);
}

uint64_t DirectXUtil::D3D12GraphicsContext::submitCommandList(SampleFramework::ID3D12GraphicsCommandList4Ptr pCmdList,
                                                              SampleFramework::ID3D12CommandQueuePtr pCmdQueue,
                                                              SampleFramework::ID3D12FencePtr pFence,
                                                              uint64_t fenceValue)
{
	pCmdList->Close();
	ID3D12CommandList* pGraphicsList = pCmdList.Get();
	pCmdQueue->ExecuteCommandLists(1, &pGraphicsList);
	fenceValue++;
	pCmdQueue->Signal(pFence.Get(), fenceValue);
	return fenceValue;
}
