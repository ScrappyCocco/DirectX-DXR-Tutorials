#include "RTPipeline.h"

#include <fstream>
#include <sstream>
#include "Source/DXSample.h"
#include "Source/DXSampleHelper.h"
#include "Source/DXCAPI/dxcapi.use.h"

const WCHAR* DirectXUtil::RTPipeline::kRayGenShader = L"rayGen";
const WCHAR* DirectXUtil::RTPipeline::kMissShader = L"miss";
const WCHAR* DirectXUtil::RTPipeline::kTriangleChs = L"triangleChs";
const WCHAR* DirectXUtil::RTPipeline::kPlaneChs = L"planeChs";
const WCHAR* DirectXUtil::RTPipeline::kTriHitGroup = L"TriHitGroup";
const WCHAR* DirectXUtil::RTPipeline::kPlaneHitGroup = L"PlaneHitGroup";
const WCHAR* DirectXUtil::RTPipeline::kShadowChs = L"shadowChs";
const WCHAR* DirectXUtil::RTPipeline::kShadowMiss = L"shadowMiss";
const WCHAR* DirectXUtil::RTPipeline::kShadowHitGroup = L"ShadowHitGroup";

static dxc::DxcDllSupport gDxcDllHelper;

DirectXUtil::Structs::DxilLibrary DirectXUtil::RTPipeline::createDxilLibrary()
{
	// Compile the shader
	const SampleFramework::IDxcBlobPtr pRayGenShader = compileLibrary(L"shaders/Shaders.hlsl", L"lib_6_3");
	const WCHAR* entryPoints[] = { kRayGenShader, kMissShader, kPlaneChs, kTriangleChs, kShadowMiss, kShadowChs };
	return DirectXUtil::Structs::DxilLibrary(pRayGenShader, entryPoints, arraysize(entryPoints));
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
	std::ifstream shaderFile(filename);
	if (shaderFile.good() == false)
	{
		ThrowError("Can't open file " + DXSample::wstring_2_string(std::wstring(filename)));
		return nullptr;
	}
	std::stringstream strStream;
	strStream << shaderFile.rdbuf();
	std::string shader = strStream.str();

	// Create blob from the string
	SampleFramework::IDxcBlobEncodingPtr pTextBlob;
	ThrowIfFailed(
		pLibrary->CreateBlobWithEncodingFromPinned(LPBYTE(shader.c_str()), static_cast<uint32_t>(shader.size()), 0, &
			pTextBlob));

	// Compile
	SampleFramework::IDxcOperationResultPtr pResult;
	ThrowIfFailed(
		pCompiler->Compile(pTextBlob.Get(), filename, L"", targetString, nullptr, 0, nullptr, 0, nullptr, &pResult));

	// Verify the result
	HRESULT resultCode;
	ThrowIfFailed(pResult->GetStatus(&resultCode));
	if (FAILED(resultCode))
	{
		SampleFramework::IDxcBlobEncodingPtr pError;
		ThrowIfFailed(pResult->GetErrorBuffer(&pError));
		const std::string log = DXSample::convertBlobToString(pError.Get());
		ThrowError("Compiler error:\n" + log);
		return nullptr;
	}

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
	desc.rootParams[0].DescriptorTable.NumDescriptorRanges = 2;
	desc.rootParams[0].DescriptorTable.pDescriptorRanges = desc.range.data();

	// Create the desc
	desc.desc.NumParameters = 1;
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	return desc;
}

DirectXUtil::RTPipeline::RootSignatureDesc DirectXUtil::RTPipeline::createTriangleHitRootDesc()
{
	RootSignatureDesc desc;
	desc.rootParams.resize(1);
	desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	desc.rootParams[0].Descriptor.RegisterSpace = 0;
	desc.rootParams[0].Descriptor.ShaderRegister = 0;

	desc.desc.NumParameters = 1;
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	return desc;
}

DirectXUtil::RTPipeline::RootSignatureDesc DirectXUtil::RTPipeline::createPlaneHitRootDesc()
{
	RootSignatureDesc desc;
	desc.range.resize(1);
	desc.range[0].BaseShaderRegister = 0;
	desc.range[0].NumDescriptors = 1;
	desc.range[0].RegisterSpace = 0;
	desc.range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[0].OffsetInDescriptorsFromTableStart = 0;

	desc.rootParams.resize(1);
	desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
	desc.rootParams[0].DescriptorTable.pDescriptorRanges = desc.range.data();

	desc.desc.NumParameters = 1;
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	return desc;
}
