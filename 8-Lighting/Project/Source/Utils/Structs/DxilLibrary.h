#pragma once

#include "Source/Dx12/InterfacePointers.h"

namespace DirectXUtil
{
	namespace Structs
	{
		struct DxilLibrary
		{
			DxilLibrary(SampleFramework::IDxcBlobPtr pBlob, const WCHAR* entryPoint[],
			            const uint32_t entryPointCount) : pShaderBlob(pBlob)
			{
				stateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
				stateSubobject.pDesc = &dxilLibDesc;

				dxilLibDesc = {};
				exportDesc.resize(entryPointCount);
				exportName.resize(entryPointCount);
				if (pBlob)
				{
					dxilLibDesc.DXILLibrary.pShaderBytecode = pBlob->GetBufferPointer();
					dxilLibDesc.DXILLibrary.BytecodeLength = pBlob->GetBufferSize();
					dxilLibDesc.NumExports = entryPointCount;
					dxilLibDesc.pExports = exportDesc.data();

					for (uint32_t i = 0; i < entryPointCount; i++)
					{
						exportName[i] = entryPoint[i];
						exportDesc[i].Name = exportName[i].c_str();
						exportDesc[i].Flags = D3D12_EXPORT_FLAG_NONE;
						exportDesc[i].ExportToRename = nullptr;
					}
				}
			}

			DxilLibrary() : DxilLibrary(nullptr, nullptr, 0)
			{
			}

			D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
			D3D12_STATE_SUBOBJECT stateSubobject{};
			SampleFramework::IDxcBlobPtr pShaderBlob;
			std::vector<D3D12_EXPORT_DESC> exportDesc;
			std::vector<std::wstring> exportName;
		};
	}
}
