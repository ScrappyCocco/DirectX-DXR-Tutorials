#include "D3D12HelloTriangle.h"

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

D3D12HelloTriangle::D3D12HelloTriangle(const UINT width, const UINT height,
                                       const std::wstring name)
	: DXSample(width, height, name)
{
	mSwapChainSize = glm::uvec2(GetWidth(), GetHeight());
}

void D3D12HelloTriangle::OnInit()
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

	createConstantBuffers();

	createShaderTable();
}

void D3D12HelloTriangle::OnUpdate()
{
	//Not used in this tutorial
}

void D3D12HelloTriangle::OnRender()
{
	const uint32_t rtvIndex = BeginFrame();

	// Refit the top-level acceleration structure
	DirectXUtil::AccelerationStructures::buildTopLevelAS(
		mpDevice,
		mpCmdList,
		mpBottomLevelAS,
		mTlasSize,
		mRotation,
		true,
		mTopLevelBuffers
	);
	mRotation += 0.005f;

	// Let's raytrace
	DirectXUtil::D3D12GraphicsContext::resourceBarrier(
		mpCmdList, mpOutputResource,
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
	raytraceDesc.MissShaderTable.SizeInBytes = mShaderTableEntrySize * 2; // 2 miss-entries

	// Hit is the fourth entry in the shader-table
	const size_t hitOffset = 3 * mShaderTableEntrySize;
	raytraceDesc.HitGroupTable.StartAddress = mpShaderTable->GetGPUVirtualAddress() + hitOffset;
	raytraceDesc.HitGroupTable.StrideInBytes = mShaderTableEntrySize;
	raytraceDesc.HitGroupTable.SizeInBytes = mShaderTableEntrySize * 8; // 8 hit-entries

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

void D3D12HelloTriangle::OnDestroy()
{
	// Wait for the command queue to finish execution
	mFenceValue++;
	mpCmdQueue->Signal(mpFence.Get(), mFenceValue);
	mpFence->SetEventOnCompletion(mFenceValue, mFenceEvent);
	WaitForSingleObject(mFenceEvent, INFINITE);
}

uint32_t D3D12HelloTriangle::BeginFrame() const
{
	// Bind the descriptor heaps
	ID3D12DescriptorHeap* heaps[] = {mpSrvUavHeap.Get()};
	mpCmdList->SetDescriptorHeaps(NV_ARRAYSIZE(heaps), heaps);
	return mpSwapChain->GetCurrentBackBufferIndex();
}

void D3D12HelloTriangle::EndFrame(const uint32_t rtvIndex)
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

	// Sync. We need to do this because the TLAS resources are not double-buffered and we are going to update them
	mpFence->SetEventOnCompletion(mFenceValue, mFenceEvent);
	WaitForSingleObject(mFenceEvent, INFINITE);

	mFrameObjects[bufferIndex].pCmdAllocator->Reset();
	mpCmdList->Reset(mFrameObjects[bufferIndex].pCmdAllocator.Get(), nullptr);
}

SampleFramework::ID3D12RootSignaturePtr D3D12HelloTriangle::createRootSignature(
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
	return pRootSig;
}

void D3D12HelloTriangle::createAccelerationStructures()
{
	mpVertexBuffer[0] = DirectXUtil::AccelerationStructures::createTriangleVB(mpDevice);
	mpVertexBuffer[1] = DirectXUtil::AccelerationStructures::createPlaneVB(mpDevice);
	DirectXUtil::Structs::AccelerationStructureBuffers bottomLevelBuffers[2];

	// The first bottom-level buffer is for the plane and the triangle
	const uint32_t vertexCount[] = {3, 6}; // Triangle has 3 vertices, plane has 6
	bottomLevelBuffers[0] = DirectXUtil::AccelerationStructures::createBottomLevelAS(
		mpDevice, mpCmdList, mpVertexBuffer, vertexCount, 2);
	mpBottomLevelAS[0] = bottomLevelBuffers[0].pResult;

	// The second bottom-level buffer is for the triangle only
	bottomLevelBuffers[1] = DirectXUtil::AccelerationStructures::createBottomLevelAS(
		mpDevice, mpCmdList, mpVertexBuffer, vertexCount, 1);
	mpBottomLevelAS[1] = bottomLevelBuffers[1].pResult;

	// Create the TLAS
	DirectXUtil::AccelerationStructures::buildTopLevelAS(
		mpDevice,
		mpCmdList,
		mpBottomLevelAS,
		mTlasSize,
		false,
		false,
		mTopLevelBuffers
	);

	// The tutorial doesn't have any resource lifetime management, so we flush and sync here.
	// This is not required by the DXR spec - you can submit the list whenever you like as long as you take care of the resources lifetime.
	mFenceValue = DirectXUtil::D3D12GraphicsContext::submitCommandList(mpCmdList, mpCmdQueue, mpFence, mFenceValue);
	mpFence->SetEventOnCompletion(mFenceValue, mFenceEvent);
	WaitForSingleObject(mFenceEvent, INFINITE);

	mpCmdList->Reset(mFrameObjects[0].pCmdAllocator.Get(), nullptr);
}

void D3D12HelloTriangle::createRtPipelineState()
{
	// Need 16 subobjects:
	//  1 for DXIL library    
	//  3 for the hit-groups (triangle hit group, plane hit-group, shadow-hit group)
	//  2 for RayGen root-signature (root-signature and the subobject association)
	//  2 for triangle hit-program root-signature (root-signature and the subobject association)
	//  2 for the plane-hit root-signature (root-signature and the subobject association)
	//  2 for shadow-program and miss root-signature (root-signature and the subobject association)
	//  2 for shader config (shared between all programs. 1 for the config, 1 for association)
	//  1 for pipeline config
	//  1 for the global root signature
	std::array<D3D12_STATE_SUBOBJECT, 16> subobjects{};
	uint32_t index = 0;

	// Create the DXIL library
	DirectXUtil::Structs::DxilLibrary dxilLib = DirectXUtil::RTPipeline::createDxilLibrary();
	subobjects[index++] = dxilLib.stateSubobject; // 0 Library

	// Create the triangle HitProgram
	DirectXUtil::Structs::HitProgram triHitProgram(nullptr, DirectXUtil::RTPipeline::kTriangleChs,
	                                               DirectXUtil::RTPipeline::kTriHitGroup);
	subobjects[index++] = triHitProgram.subObject; // 1 Triangle Hit Group

	// Create the plane HitProgram
	DirectXUtil::Structs::HitProgram planeHitProgram(nullptr, DirectXUtil::RTPipeline::kPlaneChs,
	                                                 DirectXUtil::RTPipeline::kPlaneHitGroup);
	subobjects[index++] = planeHitProgram.subObject; // 2 Plant Hit Group

	// Create the shadow-ray hit group
	DirectXUtil::Structs::HitProgram shadowHitProgram(nullptr, DirectXUtil::RTPipeline::kShadowChs,
	                                                  DirectXUtil::RTPipeline::kShadowHitGroup);
	subobjects[index++] = shadowHitProgram.subObject; // 3 Shadow Hit Group

	// Create the ray-gen root-signature and association
	DirectXUtil::Structs::LocalRootSignature rgsRootSignature(
		mpDevice, DirectXUtil::RTPipeline::createRayGenRootDesc().desc);
	subobjects[index] = rgsRootSignature.subobject; // 4 Ray Gen Root Sig

	uint32_t rgsRootIndex = index++; // 4
	DirectXUtil::Structs::ExportAssociation rgsRootAssociation(&DirectXUtil::RTPipeline::kRayGenShader, 1,
	                                                           &(subobjects[rgsRootIndex]));
	subobjects[index++] = rgsRootAssociation.subobject; // 5 Associate Root Sig to RGS

	// Create the tri hit root-signature and association
	DirectXUtil::Structs::LocalRootSignature triHitRootSignature(
		mpDevice, DirectXUtil::RTPipeline::createTriangleHitRootDesc().desc);
	subobjects[index] = triHitRootSignature.subobject; // 6 Triangle Hit Root Sig

	uint32_t triHitRootIndex = index++; // 6
	DirectXUtil::Structs::ExportAssociation triHitRootAssociation(&DirectXUtil::RTPipeline::kTriangleChs, 1,
	                                                              &(subobjects[triHitRootIndex]));
	subobjects[index++] = triHitRootAssociation.subobject; // 7 Associate Triangle Root Sig to Triangle Hit Group

	// Create the plane hit root-signature and association
	DirectXUtil::Structs::LocalRootSignature planeHitRootSignature(
		mpDevice, DirectXUtil::RTPipeline::createPlaneHitRootDesc().desc);
	subobjects[index] = planeHitRootSignature.subobject; // 8 Plane Hit Root Sig

	uint32_t planeHitRootIndex = index++; // 8
	DirectXUtil::Structs::ExportAssociation planeHitRootAssociation(&DirectXUtil::RTPipeline::kPlaneHitGroup, 1,
	                                                                &(subobjects[planeHitRootIndex]));
	subobjects[index++] = planeHitRootAssociation.subobject; // 9 Associate Plane Hit Root Sig to Plane Hit Group

	// Create the empty root-signature and associate it with the primary miss-shader and the shadow programs
	D3D12_ROOT_SIGNATURE_DESC emptyDesc = {};
	emptyDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	DirectXUtil::Structs::LocalRootSignature emptyRootSignature(mpDevice, emptyDesc);
	subobjects[index] = emptyRootSignature.subobject; // 10 Empty Root Sig for Plane Hit Group and Miss

	uint32_t emptyRootIndex = index++; // 10
	const WCHAR* emptyRootExport[] = {
		DirectXUtil::RTPipeline::kMissShader, DirectXUtil::RTPipeline::kShadowChs, DirectXUtil::RTPipeline::kShadowMiss
	};
	DirectXUtil::Structs::ExportAssociation emptyRootAssociation(emptyRootExport, NV_ARRAYSIZE(emptyRootExport),
	                                                             &(subobjects[emptyRootIndex]));
	subobjects[index++] = emptyRootAssociation.subobject;
	// 11 Associate empty root sig to Plane Hit Group and Miss shader

	// Bind the payload size to all programs
	DirectXUtil::Structs::ShaderConfig primaryShaderConfig(sizeof(float) * 2, sizeof(float) * 3);
	subobjects[index] = primaryShaderConfig.subobject; // 12

	uint32_t primaryShaderConfigIndex = index++;
	const WCHAR* primaryShaderExports[] = {
		DirectXUtil::RTPipeline::kRayGenShader,
		DirectXUtil::RTPipeline::kMissShader,
		DirectXUtil::RTPipeline::kTriangleChs,
		DirectXUtil::RTPipeline::kPlaneChs,
		DirectXUtil::RTPipeline::kShadowMiss,
		DirectXUtil::RTPipeline::kShadowChs
	};
	DirectXUtil::Structs::ExportAssociation primaryConfigAssociation(
		primaryShaderExports,
		NV_ARRAYSIZE(primaryShaderExports),
		&(subobjects[primaryShaderConfigIndex])
	);
	subobjects[index++] = primaryConfigAssociation.subobject; // 13 Associate shader config to all programs

	// Create the pipeline config
	DirectXUtil::Structs::PipelineConfig config(2);
	// maxRecursionDepth - 1 TraceRay() from the ray-gen, 1 TraceRay() from the primary hit-shader
	subobjects[index++] = config.subobject; // 14

	// Create the global root signature and store the empty signature
	DirectXUtil::Structs::GlobalRootSignature root(mpDevice, {});
	mpEmptyRootSig = root.pRootSig;
	subobjects[index++] = root.subobject; // 15

	// Create the state
	D3D12_STATE_OBJECT_DESC desc;
	desc.NumSubobjects = index; // 16
	desc.pSubobjects = subobjects.data();
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	NV_D3D_CALL(mpDevice->CreateStateObject(&desc, IID_PPV_ARGS(&mpPipelineState)));
}

void D3D12HelloTriangle::createShaderTable()
{
	/** The shader-table layout is as follows:
		Entry 0 - Ray-gen program
		Entry 1 - Miss program for the primary ray
		Entry 2 - Miss program for the shadow ray
		Entries 3,4 - Hit programs for triangle 0 (primary followed by shadow)
		Entries 5,6 - Hit programs for the plane (primary followed by shadow)
		Entries 7,8 - Hit programs for triangle 1 (primary followed by shadow)
		Entries 9,10 - Hit programs for triangle 2 (primary followed by shadow)
		All entries in the shader-table must have the same size, so we will choose it base on the largest required entry.
		The triangle primary-ray hit program requires the largest entry - sizeof(program identifier) + 8 bytes for the constant-buffer root descriptor.
		The entry size must be aligned up to D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT
	*/

	// Calculate the size and create the buffer
	mShaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	mShaderTableEntrySize += 8; // The hit shader constant-buffer descriptor
	mShaderTableEntrySize = NV_ALIGN_TO(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, mShaderTableEntrySize);
	const uint32_t shaderTableSize = mShaderTableEntrySize * 11;

	// For simplicity, we create the shader-table on the upload heap. You can also create it on the default heap
	mpShaderTable = DirectXUtil::AccelerationStructures::createBuffer(
		mpDevice, shaderTableSize,
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
	memcpy(pData + mShaderTableEntrySize, pRtsoProps->GetShaderIdentifier(DirectXUtil::RTPipeline::kMissShader),
	       D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Entry 2 - shadow-ray miss
	memcpy(pData + mShaderTableEntrySize * 2, pRtsoProps->GetShaderIdentifier(DirectXUtil::RTPipeline::kShadowMiss),
	       D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Entry 3 - Triangle 0, primary ray. ProgramID and constant-buffer data
	uint8_t* pEntry3 = pData + mShaderTableEntrySize * 3;
	memcpy(pEntry3, pRtsoProps->GetShaderIdentifier(DirectXUtil::RTPipeline::kTriHitGroup),
	       D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	assert((reinterpret_cast<uint64_t>(pEntry3 + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) % 8) == 0);
	// Root descriptor must be stored at an 8-byte aligned address
	*reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(pEntry3 + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) =
		mpConstantBuffer[0]->GetGPUVirtualAddress();

	// Entry 4 - Triangle 0, shadow ray. ProgramID only
	uint8_t* pEntry4 = pData + mShaderTableEntrySize * 4;
	memcpy(pEntry4, pRtsoProps->GetShaderIdentifier(DirectXUtil::RTPipeline::kShadowHitGroup),
	       D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Entry 5 - Plane, primary ray. ProgramID only and the TLAS SRV
	uint8_t* pEntry5 = pData + mShaderTableEntrySize * 5;
	memcpy(pEntry5, pRtsoProps->GetShaderIdentifier(DirectXUtil::RTPipeline::kPlaneHitGroup),
	       D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	*reinterpret_cast<uint64_t*>(pEntry5 + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = heapStart +
		mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	// The SRV comes directly after the program id

	// Entry 6 - Plane, shadow ray
	uint8_t* pEntry6 = pData + mShaderTableEntrySize * 6;
	memcpy(pEntry6, pRtsoProps->GetShaderIdentifier(DirectXUtil::RTPipeline::kShadowHitGroup),
	       D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Entry 7 - Triangle 1, primary ray. ProgramID and constant-buffer data
	uint8_t* pEntry7 = pData + mShaderTableEntrySize * 7;
	memcpy(pEntry7, pRtsoProps->GetShaderIdentifier(DirectXUtil::RTPipeline::kTriHitGroup),
	       D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	assert((reinterpret_cast<uint64_t>(pEntry7 + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) % 8) == 0);
	// Root descriptor must be stored at an 8-byte aligned address
	*reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(pEntry7 + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) =
		mpConstantBuffer[1]->GetGPUVirtualAddress();

	// Entry 8 - Triangle 1, shadow ray. ProgramID only
	uint8_t* pEntry8 = pData + mShaderTableEntrySize * 8;
	memcpy(pEntry8, pRtsoProps->GetShaderIdentifier(DirectXUtil::RTPipeline::kShadowHitGroup),
	       D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Entry 9 - Triangle 2, primary ray. ProgramID and constant-buffer data
	uint8_t* pEntry9 = pData + mShaderTableEntrySize * 9;
	memcpy(pEntry9, pRtsoProps->GetShaderIdentifier(DirectXUtil::RTPipeline::kTriHitGroup),
	       D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	assert((reinterpret_cast<uint64_t>(pEntry9 + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) % 8) == 0);
	// Root descriptor must be stored at an 8-byte aligned address
	*reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(pEntry9 + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) =
		mpConstantBuffer[2]->GetGPUVirtualAddress();

	// Entry 10 - Triangle 2, shadow ray. ProgramID only
	uint8_t* pEntry10 = pData + mShaderTableEntrySize * 10;
	memcpy(pEntry10, pRtsoProps->GetShaderIdentifier(DirectXUtil::RTPipeline::kShadowHitGroup),
	       D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Unmap
	mpShaderTable->Unmap(0, nullptr);
}

void D3D12HelloTriangle::createShaderResources()
{
	// Create the output resource. The dimensions and format should match the swap-chain
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	// The backbuffer is actually DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, but sRGB formats can't be used with UAVs. We will convert to sRGB ourselves in the shader
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resDesc.Height = mSwapChainSize.y;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Width = mSwapChainSize.x;
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
	// Starting as copy-source to simplify onFrameRender()

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

void D3D12HelloTriangle::createConstantBuffers()
{
	// The shader declares each CB with 3 float3. However, due to HLSL packing rules, we create the CB with vec4 (each float3 needs to start on a 16-byte boundary)
	glm::vec4 bufferData[] = {
		// Instance 0
		glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
		glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),
		glm::vec4(1.0f, 0.0f, 1.0f, 1.0f),

		// Instance 1
		glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
		glm::vec4(0.0f, 1.0f, 1.0f, 1.0f),
		glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),

		// Instance 2
		glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
		glm::vec4(1.0f, 0.0f, 1.0f, 1.0f),
		glm::vec4(0.0f, 1.0f, 1.0f, 1.0f),
	};

	for (uint32_t i = 0; i < 3; i++)
	{
		const uint32_t bufferSize = sizeof(glm::vec4) * 3;
		mpConstantBuffer[i] = DirectXUtil::AccelerationStructures::createBuffer(
			mpDevice, bufferSize, D3D12_RESOURCE_FLAG_NONE,
			D3D12_RESOURCE_STATE_GENERIC_READ, DirectXUtil::AccelerationStructures::kUploadHeapProps);
		uint8_t* pData;
		NV_D3D_CALL(mpConstantBuffer[i]->Map(0, nullptr, reinterpret_cast<void**>(&pData)));
		memcpy(pData, &bufferData[i * 3], sizeof(bufferData));
		mpConstantBuffer[i]->Unmap(0, nullptr);
	}
}
