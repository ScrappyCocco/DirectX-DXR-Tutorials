#include "AccelerationStructures.h"

#include <vector>

#include "Primitives.h"
#include "../../_externals/glm/glm/gtx/euler_angles.hpp"
#include "Source/DXSampleHelper.h"

const D3D12_HEAP_PROPERTIES DirectXUtil::AccelerationStructures::kUploadHeapProps =
{
	D3D12_HEAP_TYPE_UPLOAD,
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN,
	0,
	0,
};

const D3D12_HEAP_PROPERTIES DirectXUtil::AccelerationStructures::kDefaultHeapProps =
{
	D3D12_HEAP_TYPE_DEFAULT,
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN,
	0,
	0
};

SampleFramework::ID3D12ResourcePtr DirectXUtil::AccelerationStructures::createBuffer(
	SampleFramework::ID3D12Device5Ptr pDevice,
	const uint64_t size, const D3D12_RESOURCE_FLAGS flags,
	const D3D12_RESOURCE_STATES initState,
	const D3D12_HEAP_PROPERTIES& heapProps)
{
	D3D12_RESOURCE_DESC bufDesc;
	bufDesc.Alignment = 0;
	bufDesc.DepthOrArraySize = 1;
	bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufDesc.Flags = flags;
	bufDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufDesc.Height = 1;
	bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufDesc.MipLevels = 1;
	bufDesc.SampleDesc.Count = 1;
	bufDesc.SampleDesc.Quality = 0;
	bufDesc.Width = size;

	SampleFramework::ID3D12ResourcePtr pBuffer;
	ThrowIfFailed(pDevice->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufDesc,
		initState,
		nullptr,
		IID_PPV_ARGS(&pBuffer)
	));

	return pBuffer;
}

DirectXUtil::AccelerationStructures::ShapeResources* DirectXUtil::AccelerationStructures::createPrimitive(
	SampleFramework::ID3D12Device5Ptr pDevice)
{
	DirectXUtil::Primitives::Shape shape = DirectXUtil::Primitives::createSphere(2.0f, 32);
	//DirectXUtil::Primitives::Shape shape = DirectXUtil::Primitives::createCube(1.5f);

	ShapeResources* resources = new ShapeResources;

	//Vertex
	resources->vertexCount = static_cast<unsigned int>(shape.vertexData.size());
	resources->vertexBuffer = createBuffer(
		pDevice,
		static_cast<unsigned int>(sizeof(Structs::VertexPositionNormalTangentTexture) * shape.vertexData.size()),
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		kUploadHeapProps
	);
	uint8_t* vBuffData;
	resources->vertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vBuffData));
	memcpy(vBuffData, shape.vertexData.data(),
	       static_cast<unsigned int>(sizeof(Structs::VertexPositionNormalTangentTexture) * shape.vertexData.size()));
	resources->vertexBuffer->Unmap(0, nullptr);

	//Index
	resources->indexCount = static_cast<unsigned int>(shape.indexData.size());
	resources->indexBuffer = createBuffer(
		pDevice,
		static_cast<unsigned int>(sizeof(unsigned short) * shape.indexData.size()),
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		kUploadHeapProps
	);
	uint8_t* iBuffData;
	resources->indexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&iBuffData));
	memcpy(iBuffData, shape.indexData.data(),
	       static_cast<unsigned int>(sizeof(unsigned short) * shape.indexData.size()));
	resources->indexBuffer->Unmap(0, nullptr);

	return resources;
}

DirectXUtil::Structs::AccelerationStructureBuffers DirectXUtil::AccelerationStructures::createBottomLevelAS(
	SampleFramework::ID3D12Device5Ptr pDevice, SampleFramework::ID3D12GraphicsCommandList4Ptr pCmdList)
{
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDesc;
	//Use a single primitive in the scene for now
	geomDesc.resize(1);

	ShapeResources* shape = createPrimitive(pDevice);

	//Just one element
	for (auto& geometry_desc : geomDesc)
	{
		geometry_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geometry_desc.Triangles.VertexBuffer = D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE();
		geometry_desc.Triangles.VertexBuffer.StartAddress = shape->vertexBuffer->GetGPUVirtualAddress();
		geometry_desc.Triangles.VertexBuffer.StrideInBytes = sizeof(Structs::VertexPositionNormalTangentTexture);
		geometry_desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geometry_desc.Triangles.VertexCount = shape->vertexCount;
		geometry_desc.Triangles.IndexBuffer = shape->indexBuffer->GetGPUVirtualAddress();
		geometry_desc.Triangles.IndexCount = shape->indexCount;
		geometry_desc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
		geometry_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	}

	// Get the size requirements for the scratch and AS buffers
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = geomDesc.size();
	inputs.pGeometryDescs = geomDesc.data();
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	// Create the buffers. They need to support UAV, and since we are going to immediately use them, we create them with an unordered-access state
	DirectXUtil::Structs::AccelerationStructureBuffers buffers;
	buffers.pScratch = createBuffer(
		pDevice,
		info.ScratchDataSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		kDefaultHeapProps
	);

	buffers.pResult = createBuffer(
		pDevice,
		info.ResultDataMaxSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		kDefaultHeapProps
	);

	// Create the bottom-level AS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.DestAccelerationStructureData = buffers.pResult->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = buffers.pScratch->GetGPUVirtualAddress();

	pCmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = buffers.pResult.Get();
	pCmdList->ResourceBarrier(1, &uavBarrier);

	return buffers;
}

void DirectXUtil::AccelerationStructures::buildTopLevelAS(SampleFramework::ID3D12Device5Ptr pDevice,
                                                          SampleFramework::ID3D12GraphicsCommandList4Ptr pCmdList,
                                                          SampleFramework::ID3D12ResourcePtr pBottomLevelAS,
                                                          uint64_t& tlasSize,
                                                          DirectXUtil::Structs::AccelerationStructureBuffers& buffers)
{
	// First, get the size of the TLAS buffers and create them
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = 1;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	// Create the buffers
	buffers.pScratch = createBuffer(
		pDevice,
		info.ScratchDataSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		kDefaultHeapProps
	);
	buffers.pResult = createBuffer(
		pDevice,
		info.ResultDataMaxSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		kDefaultHeapProps);
	buffers.pInstanceDesc = createBuffer(
		pDevice,
		sizeof(D3D12_RAYTRACING_INSTANCE_DESC),
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		kUploadHeapProps
	);
	tlasSize = info.ResultDataMaxSizeInBytes;

	// Map the instance desc buffer
	D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs;
	buffers.pInstanceDesc->Map(0, nullptr, reinterpret_cast<void**>(&instanceDescs));
	ZeroMemory(instanceDescs, sizeof(D3D12_RAYTRACING_INSTANCE_DESC));

	// The InstanceContributionToHitGroupIndex is set based on the shader-table layout specified in createShaderTable()
	// Create the desc for the triangle/plane instance
	instanceDescs[0].InstanceID = 0;
	instanceDescs[0].InstanceContributionToHitGroupIndex = 0;
	instanceDescs[0].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	// The transformation matrix
	glm::mat4 transform(1.0);
	memcpy(instanceDescs[0].Transform, &transform[0], sizeof(instanceDescs[0].Transform));
	instanceDescs[0].AccelerationStructure = pBottomLevelAS->GetGPUVirtualAddress();
	instanceDescs[0].InstanceMask = 0xFF;

	// Unmap
	buffers.pInstanceDesc->Unmap(0, nullptr);

	// Create the TLAS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.Inputs.InstanceDescs = buffers.pInstanceDesc->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = buffers.pResult->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = buffers.pScratch->GetGPUVirtualAddress();

	pCmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = buffers.pResult.Get();
	pCmdList->ResourceBarrier(1, &uavBarrier);
}
