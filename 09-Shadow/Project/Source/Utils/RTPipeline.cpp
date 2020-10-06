#include "RTPipeline.h"

#include <fstream>
#include <sstream>
#include "Source/DXSample.h"
#include "Source/DXSampleHelper.h"
#include "Source/DXCAPI/dxcapi.use.h"

const WCHAR* DirectXUtil::RTPipeline::kRayGenShader = L"rayGen";
const WCHAR* DirectXUtil::RTPipeline::kMissShader = L"miss";
const WCHAR* DirectXUtil::RTPipeline::kClosestHitShader = L"chs";
const WCHAR* DirectXUtil::RTPipeline::kHitGroup = L"HitGroup";
const WCHAR* DirectXUtil::RTPipeline::kShadowMiss = L"shadowMiss";

static dxc::DxcDllSupport gDxcDllHelper;

DirectXUtil::Structs::DxilLibrary DirectXUtil::RTPipeline::createDxilLibrary()
{
	// Compile the shader
	const SampleFramework::IDxcBlobPtr pRayGenShader = compileLibrary(L"shaders/Shaders.hlsl", L"lib_6_3");
	const WCHAR* entryPoints[] = {kRayGenShader, kMissShader, kClosestHitShader, kShadowMiss};
	return DirectXUtil::Structs::DxilLibrary(pRayGenShader, entryPoints, NV_ARRAYSIZE(entryPoints));
}

SampleFramework::IDxcBlobPtr DirectXUtil::RTPipeline::compileLibrary(const WCHAR* filename, const WCHAR* targetString)
{
	// Initialize the helper
	ThrowIfFailed(gDxcDllHelper.Initialize());
	SampleFramework::IDxcCompilerPtr pCompiler;
	SampleFramework::IDxcLibraryPtr pLibrary;
	ThrowIfFailed(gDxcDllHelper.CreateInstance(CLSID_DxcCompiler, pCompiler.GetAddressOf()));
	ThrowIfFailed(gDxcDllHelper.CreateInstance(CLSID_DxcLibrary, pLibrary.GetAddressOf()));

	// Open and read the file
	const std::ifstream shaderFile(filename);
	if (shaderFile.good() == false)
	{
		ThrowError("Can't open file " + DXSample::wstring_2_string(std::wstring(filename)));
	}
	std::stringstream strStream;
	strStream << shaderFile.rdbuf();
	const std::string shader = strStream.str();

	// Create blob from the string
	SampleFramework::IDxcBlobEncodingPtr pTextBlob;
	ThrowIfFailed(
		pLibrary->CreateBlobWithEncodingFromPinned(
			LPBYTE(shader.c_str()),
			static_cast<uint32_t>(shader.size()),
			0,
			&pTextBlob
		)
	);

	//Create include handler
	SampleFramework::IDxcIncludeHandlerPtr includeHandler;
	pLibrary->CreateIncludeHandler(includeHandler.GetAddressOf());

	// Compile
	SampleFramework::IDxcOperationResultPtr pResult;
	ThrowIfFailed(
		pCompiler->Compile(
			pTextBlob.Get(),
			filename,
			L"",
			targetString,
			nullptr,
			0,
			nullptr,
			0,
			includeHandler.Get(),
			&pResult
		)
	);

	// Verify the result
	HRESULT resultCode;
	ThrowIfFailed(pResult->GetStatus(&resultCode));

	SampleFramework::IDxcBlobPtr pBlob;
	ThrowIfFailed(pResult->GetResult(&pBlob));
	return pBlob;
}

DirectXUtil::RTPipeline::RootSignatureDesc DirectXUtil::RTPipeline::createRayGenRootDesc()
{
	// Create the root-signature
	RootSignatureDesc desc;
	desc.range.resize(2);

	// gOutput
	desc.range[0].BaseShaderRegister = 0;
	desc.range[0].NumDescriptors = 1;
	desc.range[0].RegisterSpace = 0;
	desc.range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	desc.range[0].OffsetInDescriptorsFromTableStart = 0;

	// gRtScene
	desc.range[1].BaseShaderRegister = 0;
	desc.range[1].NumDescriptors = 1;
	desc.range[1].RegisterSpace = 0;
	desc.range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[1].OffsetInDescriptorsFromTableStart = 1;

	desc.rootParams.resize(1);
	desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[0].DescriptorTable.NumDescriptorRanges = desc.range.size();
	desc.rootParams[0].DescriptorTable.pDescriptorRanges = desc.range.data();

	// Create the desc
	desc.desc.NumParameters = 1;
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	return desc;
}

DirectXUtil::RTPipeline::RootSignatureDesc DirectXUtil::RTPipeline::CreateHitRootDesc()
{
	// Create the root-signature
	RootSignatureDesc desc;
	desc.range.resize(3);

	// gRtScene
	desc.range[0].BaseShaderRegister = 0;
	desc.range[0].NumDescriptors = 1;
	desc.range[0].RegisterSpace = 0;
	desc.range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[0].OffsetInDescriptorsFromTableStart = 1;

	// Indices
	desc.range[1].BaseShaderRegister = 1;
	desc.range[1].NumDescriptors = 1;
	desc.range[1].RegisterSpace = 0;
	desc.range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[1].OffsetInDescriptorsFromTableStart = 2;

	// Vertices
	desc.range[2].BaseShaderRegister = 2;
	desc.range[2].NumDescriptors = 1;
	desc.range[2].RegisterSpace = 0;
	desc.range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[2].OffsetInDescriptorsFromTableStart = 3;

	desc.rootParams.resize(1);
	desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[0].DescriptorTable.NumDescriptorRanges = desc.range.size();
	desc.rootParams[0].DescriptorTable.pDescriptorRanges = desc.range.data();

	// Create the desc
	desc.desc.NumParameters = 1;
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	return desc;
}
