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
		static const WCHAR* kTriangleChs;
		static const WCHAR* kPlaneChs;
		static const WCHAR* kTriHitGroup;
		static const WCHAR* kPlaneHitGroup;
		static const WCHAR* kShadowChs;
		static const WCHAR* kShadowMiss;
		static const WCHAR* kShadowHitGroup;

		static DirectXUtil::Structs::DxilLibrary createDxilLibrary();
		static SampleFramework::IDxcBlobPtr compileLibrary(const WCHAR* filename, const WCHAR* targetString);

		static RootSignatureDesc createRayGenRootDesc();
		static RootSignatureDesc createTriangleHitRootDesc();
		static RootSignatureDesc createPlaneHitRootDesc();
	};
}
