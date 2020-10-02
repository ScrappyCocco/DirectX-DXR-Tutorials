#pragma once

#include <cstdint>
#include <d3d12.h>

namespace DirectXUtil
{
	namespace Structs
	{
		struct ExportAssociation
		{
			ExportAssociation(const WCHAR* exportNames[], const uint32_t exportCount,
			                  const D3D12_STATE_SUBOBJECT* pSubobjectToAssociate)
			{
				association.NumExports = exportCount;
				association.pExports = exportNames;
				association.pSubobjectToAssociate = pSubobjectToAssociate;

				subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
				subobject.pDesc = &association;
			}

			D3D12_STATE_SUBOBJECT subobject = {};
			D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association = {};
		};
	}
}
