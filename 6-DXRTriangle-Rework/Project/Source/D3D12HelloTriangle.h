//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include "DXSample.h"

#include "Dx12/InterfacePointers.h"
#define GLM_FORCE_CTOR_INIT
#define GLM_ENABLE_EXPERIMENTAL
#include "../../_externals/glm/glm/glm.hpp"
#include <vector>
#include "Utils/D3D12GraphicsContext.h"
#include "Utils/Structs/AccelerationStructureBuffers.h"
#include "Utils/Structs/FrameObject.h"
#include "Utils/Structs/HeapData.h"

using Microsoft::WRL::ComPtr;

class D3D12HelloTriangle final : public DXSample
{
public:
	D3D12HelloTriangle(UINT width, UINT height, std::wstring name);

	void OnInit() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnDestroy() override;

	static SampleFramework::ID3D12RootSignaturePtr createRootSignature(
		SampleFramework::ID3D12Device5Ptr pDevice,
		const D3D12_ROOT_SIGNATURE_DESC& desc
	);

private:
	SampleFramework::ID3D12Device5Ptr mpDevice;
	SampleFramework::ID3D12CommandQueuePtr mpCmdQueue;
	SampleFramework::IDXGISwapChain3Ptr mpSwapChain;
	glm::uvec2 mSwapChainSize;
	SampleFramework::ID3D12GraphicsCommandList4Ptr mpCmdList;
	SampleFramework::ID3D12FencePtr mpFence;
	HANDLE mFenceEvent;
	uint64_t mFenceValue = 0;

	std::vector <DirectXUtil::Structs::FrameObject> mFrameObjects;

	DirectXUtil::Structs::HeapData mRtvHeap;
	static const uint32_t kRtvHeapSize = 3;

	//-----------------------------------------------
	// DXR HELPER STUFF
	//-----------------------------------------------

	DirectXUtil::D3D12GraphicsContext D3D12UtilContext;

	uint32_t BeginFrame() const;
	void EndFrame(uint32_t rtvIndex);

	//-----------------------------------------------
	// DXR FUNCTIONS
	//-----------------------------------------------

	void createAccelerationStructures();
	SampleFramework::ID3D12ResourcePtr mpVertexBuffer[2];
	SampleFramework::ID3D12ResourcePtr mpBottomLevelAS[2];
	DirectXUtil::Structs::AccelerationStructureBuffers mTopLevelBuffers;
	uint64_t mTlasSize = 0;

	void createRtPipelineState();
	SampleFramework::ID3D12StateObjectPtr mpPipelineState;
	SampleFramework::ID3D12RootSignaturePtr mpEmptyRootSig;

	void createShaderTable();
	SampleFramework::ID3D12ResourcePtr mpShaderTable;
	uint32_t mShaderTableEntrySize = 0;

	void createShaderResources();
	SampleFramework::ID3D12ResourcePtr mpOutputResource;
	SampleFramework::ID3D12DescriptorHeapPtr mpSrvUavHeap;
	static const uint32_t kSrvUavHeapSize = 2;

	void createConstantBuffers();
	SampleFramework::ID3D12ResourcePtr mpConstantBuffer[3];

	float mRotation = 0;
};
