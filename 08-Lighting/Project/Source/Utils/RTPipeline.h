#pragma once

#include <vector>
#include "Structs/DxilLibrary.h"

namespace DirectXUtil
{
	class RTPipeline
	{
	public:
		struct RootSignatureDesc
		{
			D3D12_ROOT_SIGNATURE_DESC desc = {};
			std::vector<D3D12_DESCRIPTOR_RANGE> range;
			std::vector<D3D12_ROOT_PARAMETER> rootParams;
		};

		static const WCHAR* kRayGenShader;
		static const WCHAR* kMissShader;
		static const WCHAR* kClosestHitShader;
		static const WCHAR* kHitGroup;

		static DirectXUtil::Structs::DxilLibrary createDxilLibrary();
		static SampleFramework::IDxcBlobPtr compileLibrary(const WCHAR* filename, const WCHAR* targetString);

		static RootSignatureDesc createRayGenRootDesc();
		static RootSignatureDesc CreateHitRootDesc();
	};
}
