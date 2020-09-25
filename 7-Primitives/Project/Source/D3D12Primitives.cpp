#include "D3D12Primitives.h"

#include "stdafx.h"
#include "Win32Application.h"
#include "../../_externals/glm/glm/gtx/transform.hpp"
#include "../../_externals/glm/glm/gtx/euler_angles.hpp"
#include <array>
#include "Utils/AccelerationStructures.h"
#include "Utils/RTPipeline.h"
#include "Utils/Structs/ExportAssociation.h"
#include "Utils/Structs/GlobalRootSignature.h"
#include "Utils/Structs/HitProgram.h"
#include "Utils/Structs/LocalRootSignature.h"
#include "Utils/Structs/PipelineConfig.h"
#include "Utils/Structs/ShaderConfig.h"

D3D12Primitives::D3D12Primitives(const UINT width, const UINT height,
                                 const std::wstring name)
	: DXSample(width, height, name)
{
	mSwapChainSize = glm::uvec2(GetWidth(), GetHeight());
}

void D3D12Primitives::OnInit()
{
	const HWND mHwnd = Win32Application::GetHwnd();
	const UINT winWidth = GetWidth();
	const UINT winHeight = GetHeight();

	// Debug Layer
#ifdef _DEBUG
	SampleFramework::ID3D12Debug1Ptr pDx12Debug;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDx12Debug))))
	{
		pDx12Debug->SetEnableGPUBasedValidation(true);
		pDx12Debug->SetEnableSynchronizedCommandQueueValidation(true);
		pDx12Debug->EnableDebugLayer();
	}

#endif

	// Create the DXGI factory
	SampleFramework::IDXGIFactory4Ptr pDxgiFactory;
	NV_D3D_CALL(CreateDXGIFactory1(IID_PPV_ARGS(&pDxgiFactory)));
	mpDevice = D3D12UtilContext.createDevice(pDxgiFactory);
	mpCmdQueue = D3D12UtilContext.createCommandQueue(mpDevice);
	mpSwapChain = D3D12UtilContext.createDxgiSwapChain(pDxgiFactory, mHwnd, winWidth, winHeight,
	                                                   DXGI_FORMAT_R8G8B8A8_UNORM, mpCmdQueue);

	// Create a RTV descriptor heap
	mRtvHeap.pHeap = D3D12UtilContext.createDescriptorHeap(mpDevice, kRtvHeapSize, D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
	                                                       false);

	// Create the per-frame objects
	mFrameObjects.resize(DirectXUtil::D3D12GraphicsContext::getDefaultSwapChainBuffers());
	for (uint32_t i = 0; i < mFrameObjects.size(); i++)
	{
		NV_D3D_CALL(
			mpDevice->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(
					&mFrameObjects[i].pCmdAllocator
				)
			)
		);

		NV_D3D_CALL(
			mpSwapChain->GetBuffer(
				i,
				IID_PPV_ARGS(
					&mFrameObjects[i].pSwapChainBuffer
				)
			)
		);

		mFrameObjects[i].rtvHandle = DirectXUtil::D3D12GraphicsContext::createRTV(
			mpDevice,
			mFrameObjects[i].pSwapChainBuffer,
			mRtvHeap.pHeap,
			mRtvHeap.usedEntries,
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
		);
	}

	// Create the command-list
	NV_D3D_CALL(
		mpDevice->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			mFrameObjects[0].pCmdAllocator.Get(),
			nullptr,
			IID_PPV_ARGS(&mpCmdList)
		)
	);

	// Create a fence and the event
	NV_D3D_CALL(mpDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mpFence)));
	mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	createAccelerationStructures();

	createRtPipelineState();

	createShaderResources();

	createShaderTable();
}

void D3D12Primitives::OnUpdate()
{
	//Not used in this tutorial
}

void D3D12Primitives::OnRender()
{
	const uint32_t rtvIndex = BeginFrame();

	// Let's raytrace
	DirectXUtil::D3D12GraphicsContext::resourceBarrier(
		mpCmdList,
		mpOutputResource,
		D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);
	D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
	raytraceDesc.Width = mSwapChainSize.x;
	raytraceDesc.Height = mSwapChainSize.y;
	raytraceDesc.Depth = 1;

	// RayGen is the first entry in the shader-table
	raytraceDesc.RayGenerationShaderRecord.StartAddress =
		mpShaderTable->GetGPUVirtualAddress() + 0 * mShaderTableEntrySize;
	raytraceDesc.RayGenerationShaderRecord.SizeInBytes = mShaderTableEntrySize;

	// Miss is the second entry in the shader-table
	const size_t missOffset = 1 * mShaderTableEntrySize;
	raytraceDesc.MissShaderTable.StartAddress = mpShaderTable->GetGPUVirtualAddress() + missOffset;
	raytraceDesc.MissShaderTable.StrideInBytes = mShaderTableEntrySize;
	raytraceDesc.MissShaderTable.SizeInBytes = mShaderTableEntrySize * 1; // Only a s single miss-entry 

	// Hit is the fourth entry in the shader-table
	const size_t hitOffset = 2 * mShaderTableEntrySize;
	raytraceDesc.HitGroupTable.StartAddress = mpShaderTable->GetGPUVirtualAddress() + hitOffset;
	raytraceDesc.HitGroupTable.StrideInBytes = mShaderTableEntrySize;
	raytraceDesc.HitGroupTable.SizeInBytes = mShaderTableEntrySize * 1;

	// Bind the empty root signature
	mpCmdList->SetComputeRootSignature(mpEmptyRootSig.Get());

	// Dispatch
	mpCmdList->SetPipelineState1(mpPipelineState.Get());
	mpCmdList->DispatchRays(&raytraceDesc);

	// Copy the results to the back-buffer
	DirectXUtil::D3D12GraphicsContext::resourceBarrier(
		mpCmdList,
		mpOutputResource,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COPY_SOURCE
	);
	DirectXUtil::D3D12GraphicsContext::resourceBarrier(
		mpCmdList,
		mFrameObjects[rtvIndex].pSwapChainBuffer,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_COPY_DEST
	);
	mpCmdList->CopyResource(mFrameObjects[rtvIndex].pSwapChainBuffer.Get(), mpOutputResource.Get());

	EndFrame(rtvIndex);
}

void D3D12Primitives::OnDestroy()
{
	// Wait for the command queue to finish execution
	mFenceValue++;
	mpCmdQueue->Signal(mpFence.Get(), mFenceValue);
	mpFence->SetEventOnCompletion(mFenceValue, mFenceEvent);
	WaitForSingleObject(mFenceEvent, INFINITE);
}

uint32_t D3D12Primitives::BeginFrame() const
{
	// Bind the descriptor heaps
	ID3D12DescriptorHeap* heaps[] = {mpSrvUavHeap.Get()};
	mpCmdList->SetDescriptorHeaps(NV_ARRAYSIZE(heaps), heaps);
	return mpSwapChain->GetCurrentBackBufferIndex();
}

void D3D12Primitives::EndFrame(const uint32_t rtvIndex)
{
	DirectXUtil::D3D12GraphicsContext::resourceBarrier(
		mpCmdList,
		mFrameObjects[rtvIndex].pSwapChainBuffer,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PRESENT
	);
	mFenceValue = DirectXUtil::D3D12GraphicsContext::submitCommandList(mpCmdList, mpCmdQueue, mpFence, mFenceValue);
	mpSwapChain->Present(0, 0);

	// Prepare the command list for the next frame
	const uint32_t bufferIndex = mpSwapChain->GetCurrentBackBufferIndex();

	// Make sure we have the new back-buffer is ready
	const uint32_t kDefaultSwapChainBuffers = DirectXUtil::D3D12GraphicsContext::getDefaultSwapChainBuffers();
	if (mFenceValue > kDefaultSwapChainBuffers)
	{
		mpFence->SetEventOnCompletion(mFenceValue - kDefaultSwapChainBuffers + 1, mFenceEvent);
		WaitForSingleObject(mFenceEvent, INFINITE);
	}

	mFrameObjects[bufferIndex].pCmdAllocator->Reset();
	mpCmdList->Reset(mFrameObjects[bufferIndex].pCmdAllocator.Get(), nullptr);
}

SampleFramework::ID3D12RootSignaturePtr D3D12Primitives::createRootSignature(
	SampleFramework::ID3D12Device5Ptr pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
{
	SampleFramework::ID3DBlobPtr pSigBlob;
	SampleFramework::ID3DBlobPtr pErrorBlob;
	const HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &pSigBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		const std::string msg = convertBlobToString(pErrorBlob.Get());
		msgBox(msg);
		return nullptr;
	}
	SampleFramework::ID3D12RootSignaturePtr pRootSig;
	NV_D3D_CALL(
		pDevice->CreateRootSignature(
			0,
			pSigBlob->GetBufferPointer(),
			pSigBlob->GetBufferSize(),
			IID_PPV_ARGS(&pRootSig)
		)
	);
	auto result = pDevice->GetDeviceRemovedReason();
	return pRootSig;
}

void D3D12Primitives::createAccelerationStructures()
{
	const DirectXUtil::Structs::AccelerationStructureBuffers bottomLevelBuffer =
		DirectXUtil::AccelerationStructures::createBottomLevelAS(
			mpDevice,
			mpCmdList
		);
	mpBottomLevelAS = bottomLevelBuffer.pResult;

	// Create the TLAS
	DirectXUtil::AccelerationStructures::buildTopLevelAS(
		mpDevice,
		mpCmdList,
		mpBottomLevelAS,
		mTlasSize,
		mTopLevelBuffers
	);

	// The tutorial doesn't have any resource lifetime management, so we flush and sync here.
	// This is not required by the DXR spec - you can submit the list whenever you like as long as you take care of the resources lifetime.
	mFenceValue = DirectXUtil::D3D12GraphicsContext::submitCommandList(mpCmdList, mpCmdQueue, mpFence, mFenceValue);

	mpFence->SetEventOnCompletion(mFenceValue, mFenceEvent);
	WaitForSingleObject(mFenceEvent, INFINITE);

	mpCmdList->Reset(mFrameObjects[0].pCmdAllocator.Get(), nullptr);
}

void D3D12Primitives::createRtPipelineState()
{
	// Need 10 subobjects:
	//  1 for the DXIL library
	//  1 for hit-group
	//  2 for RayGen root-signature (root-signature and the subobject association)
	//  2 for the root-signature shared between miss and hit shaders (signature and association)
	//  2 for shader config (shared between all programs. 1 for the config, 1 for association)
	//  1 for pipeline config
	//  1 for the global root signature
	std::array<D3D12_STATE_SUBOBJECT, 10> subobjects{};
	uint32_t index = 0;

	// Create the DXIL library
	DirectXUtil::Structs::DxilLibrary dxilLib = DirectXUtil::RTPipeline::createDxilLibrary();
	subobjects[index++] = dxilLib.stateSubobject; // 0 Library

	DirectXUtil::Structs::HitProgram hitProgram(nullptr, DirectXUtil::RTPipeline::kClosestHitShader,
	                                            DirectXUtil::RTPipeline::kHitGroup);
	subobjects[index++] = hitProgram.subObject; // 1 Hit Group

	// Create the ray-gen root-signature and association
	DirectXUtil::Structs::LocalRootSignature rgsRootSignature(
		mpDevice, DirectXUtil::RTPipeline::createRayGenRootDesc().desc);
	subobjects[index] = rgsRootSignature.subobject; // 2 RayGen Root Sig

	uint32_t rgsRootIndex = index++; // 2
	DirectXUtil::Structs::ExportAssociation rgsRootAssociation(&DirectXUtil::RTPipeline::kRayGenShader, 1,
	                                                           &(subobjects[rgsRootIndex]));
	subobjects[index++] = rgsRootAssociation.subobject; // 3 Associate Root Sig to RGS

	// Create the empty root-signature and associate it with the primary miss-shader and the shadow programs
	D3D12_ROOT_SIGNATURE_DESC emptyDesc = {};
	emptyDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	DirectXUtil::Structs::LocalRootSignature hitMissRootSignature(mpDevice, emptyDesc);
	subobjects[index] = hitMissRootSignature.subobject; // 4 Root Sig to be shared between Miss and CHS

	uint32_t hitMissRootIndex = index++; // 4
	const WCHAR* missHitExportName[] = {
		DirectXUtil::RTPipeline::kMissShader, DirectXUtil::RTPipeline::kClosestHitShader
	};
	DirectXUtil::Structs::ExportAssociation missHitRootAssociation(missHitExportName, NV_ARRAYSIZE(missHitExportName),
	                                                               &(subobjects[hitMissRootIndex]));
	subobjects[index++] = missHitRootAssociation.subobject; // 5 Associate Root Sig to Miss and CHS

	// Bind the payload size to all programs
	DirectXUtil::Structs::ShaderConfig primaryShaderConfig(sizeof(float) * 2, sizeof(float) * 4);
	subobjects[index] = primaryShaderConfig.subobject; // 6 Shader Config;

	uint32_t primaryShaderConfigIndex = index++; // 6
	const WCHAR* primaryShaderExports[] = {
		DirectXUtil::RTPipeline::kMissShader,
		DirectXUtil::RTPipeline::kClosestHitShader,
		DirectXUtil::RTPipeline::kRayGenShader
	};
	DirectXUtil::Structs::ExportAssociation primaryConfigAssociation(
		primaryShaderExports,
		NV_ARRAYSIZE(primaryShaderExports),
		&(subobjects[primaryShaderConfigIndex])
	);
	subobjects[index++] = primaryConfigAssociation.subobject; // 7 Associate Shader Config to Miss, CHS, RGS

	// Create the pipeline config
	DirectXUtil::Structs::PipelineConfig config(1);
	// maxRecursionDepth - 1 TraceRay() from the ray-gen, 1 TraceRay() from the primary hit-shader
	subobjects[index++] = config.subobject; // 8

	// Create the global root signature and store the empty signature
	DirectXUtil::Structs::GlobalRootSignature root(mpDevice, {});
	mpEmptyRootSig = root.pRootSig;
	subobjects[index++] = root.subobject; // 9

	// Create the state
	D3D12_STATE_OBJECT_DESC desc;
	desc.NumSubobjects = index;
	desc.pSubobjects = subobjects.data();
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	NV_D3D_CALL(mpDevice->CreateStateObject(&desc, IID_PPV_ARGS(&mpPipelineState)));
}

void D3D12Primitives::createShaderTable()
{
	/** The shader-table layout is as follows:
		Entry 0 - Ray-gen program
		Entry 1 - Miss program
		Entry 2 - Hit program
		All entries in the shader-table must have the same size, so we will choose it base on the largest required entry.
		The ray-gen program requires the largest entry - sizeof(program identifier) + 8 bytes for a descriptor-table.
		The entry size must be aligned up to D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT
	*/

	// Calculate the size and create the buffer
	mShaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	mShaderTableEntrySize += 8; // The hit shader constant-buffer descriptor
	mShaderTableEntrySize = NV_ALIGN_TO(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, mShaderTableEntrySize);
	const uint32_t shaderTableSize = mShaderTableEntrySize * 3;

	// For simplicity, we create the shader-table on the upload heap. You can also create it on the default heap
	mpShaderTable = DirectXUtil::AccelerationStructures::createBuffer(
		mpDevice,
		shaderTableSize,
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		DirectXUtil::AccelerationStructures::kUploadHeapProps
	);

	// Map the buffer
	uint8_t* pData;
	NV_D3D_CALL(mpShaderTable->Map(0, nullptr, reinterpret_cast<void**>(&pData)));

	SampleFramework::ID3D12StateObjectPropertiesPtr pRtsoProps;
	mpPipelineState->QueryInterface(IID_PPV_ARGS(&pRtsoProps));

	// Entry 0 - ray-gen program ID and descriptor data
	memcpy(pData, pRtsoProps->GetShaderIdentifier(DirectXUtil::RTPipeline::kRayGenShader),
	       D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	const uint64_t heapStart = mpSrvUavHeap->GetGPUDescriptorHandleForHeapStart().ptr;
	*reinterpret_cast<uint64_t*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = heapStart;

	// Entry 1 - primary ray miss
	pData += mShaderTableEntrySize;
	memcpy(pData, pRtsoProps->GetShaderIdentifier(DirectXUtil::RTPipeline::kMissShader),
	       D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Entry 2 - hit program
	pData += mShaderTableEntrySize;
	memcpy(pData, pRtsoProps->GetShaderIdentifier(DirectXUtil::RTPipeline::kHitGroup),
	       D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Unmap
	mpShaderTable->Unmap(0, nullptr);
}

void D3D12Primitives::createShaderResources()
{
	// Create the output resource. The dimensions and format should match the swap-chain
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	// The backbuffer is actually DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, but sRGB formats can't be used with UAVs. We will convert to sRGB ourselves in the shader
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resDesc.Width = mSwapChainSize.x;
	resDesc.Height = mSwapChainSize.y;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;

	NV_D3D_CALL(
		mpDevice->CreateCommittedResource(
			&DirectXUtil::AccelerationStructures::kDefaultHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			nullptr,
			IID_PPV_ARGS(&mpOutputResource)
		)
	);

	// Create an SRV/UAV descriptor heap. Need 2 entries - 1 SRV for the scene and 1 UAV for the output
	mpSrvUavHeap = D3D12UtilContext.createDescriptorHeap(mpDevice, 2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	// Create the UAV. Based on the root signature we created it should be the first entry
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	mpDevice->CreateUnorderedAccessView(
		mpOutputResource.Get(),
		nullptr,
		&uavDesc,
		mpSrvUavHeap->GetCPUDescriptorHandleForHeapStart()
	);

	// Create the TLAS SRV right after the UAV. Note that we are using a different SRV desc here
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location = mTopLevelBuffers.pResult->GetGPUVirtualAddress();
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = mpSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
	srvHandle.ptr += mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mpDevice->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);
}
