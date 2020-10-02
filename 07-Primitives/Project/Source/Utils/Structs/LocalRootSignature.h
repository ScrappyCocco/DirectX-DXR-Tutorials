#pragma once

#include <d3d12.h>
#include "Source/Dx12/InterfacePointers.h"

namespace DirectXUtil
{
	namespace Structs
	{
		struct LocalRootSignature
		{
			LocalRootSignature(const SampleFramework::ID3D12Device5Ptr pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
			{
				pRootSig = D3D12Primitives::createRootSignature(pDevice, desc);
				pInterface = pRootSig.Get();
				subobject.pDesc = &pInterface;
				subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
			}

			SampleFramework::ID3D12RootSignaturePtr pRootSig;
			ID3D12RootSignature* pInterface = nullptr;
			D3D12_STATE_SUBOBJECT subobject = {};
		};
	}
}
