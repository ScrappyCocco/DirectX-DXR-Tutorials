#include "AccelerationStructures.h"

#include <vector>

#include "Primitives.h"
#include "../../_externals/glm/glm/gtx/euler_angles.hpp"
#include "Source/DXSampleHelper.h"

//Init static instance reference

DirectXUtil::AccelerationStructures::ShapeResources* DirectXUtil::AccelerationStructures::createdPrimitive = nullptr;

//Class

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
	SampleFramework::ID3D12Device5Ptr pDevice, const PrimitiveType type)
{
	DirectXUtil::Primitives::Shape shape;

	switch (type)
	{
	case SPHERE:
		shape = DirectXUtil::Primitives::createSphere(2.0f, 32);
		break;
	case CUBE:
		shape = DirectXUtil::Primitives::createCube(1.5f);
		break;
	case QUAD:
		shape = DirectXUtil::Primitives::createQuad(100);
		break;
	}

	ShapeResources* primitiveData = new ShapeResources;

	//Vertex
	primitiveData->vertexCount = static_cast<unsigned int>(shape.vertexData.size());
	primitiveData->vertexBuffer = createBuffer(
		pDevice,
		static_cast<unsigned int>(sizeof(Structs::VertexPositionNormalTangentTexture) * shape.vertexData.size()),
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		kUploadHeapProps
	);
	uint8_t* vBuffData;
	primitiveData->vertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vBuffData));
	memcpy(vBuffData, shape.vertexData.data(),
	       static_cast<unsigned int>(sizeof(Structs::VertexPositionNormalTangentTexture) * shape.vertexData.size()));
	primitiveData->vertexBuffer->Unmap(0, nullptr);

	//Index
	primitiveData->indexCount = static_cast<unsigned int>(shape.indexData.size());
	primitiveData->indexBuffer = createBuffer(
		pDevice,
		static_cast<unsigned int>(sizeof(unsigned short) * shape.indexData.size()),
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		kUploadHeapProps
	);
	uint8_t* iBuffData;
	primitiveData->indexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&iBuffData));
	memcpy(iBuffData, shape.indexData.data(),
	       static_cast<unsigned int>(sizeof(unsigned short) * shape.indexData.size()));
	primitiveData->indexBuffer->Unmap(0, nullptr);

	return primitiveData;
}

DirectXUtil::Structs::AccelerationStructureBuffers DirectXUtil::AccelerationStructures::createBottomLevelAS(
	SampleFramework::ID3D12Device5Ptr pDevice, SampleFramework::ID3D12GraphicsCommandList4Ptr pCmdList,
	ShapeResources* shape)
{
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDesc;
	//Single shape AS
	geomDesc.resize(1);

	//Just one element
	geomDesc[0].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geomDesc[0].Triangles.VertexBuffer = D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE();
	geomDesc[0].Triangles.VertexBuffer.StartAddress = shape->vertexBuffer->GetGPUVirtualAddress();
	geomDesc[0].Triangles.VertexBuffer.StrideInBytes = sizeof(Structs::VertexPositionNormalTangentTexture);
	geomDesc[0].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geomDesc[0].Triangles.VertexCount = shape->vertexCount;
	geomDesc[0].Triangles.IndexBuffer = shape->indexBuffer->GetGPUVirtualAddress();
	geomDesc[0].Triangles.IndexCount = shape->indexCount;
	geomDesc[0].Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
	geomDesc[0].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

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

DirectXUtil::Structs::AccelerationStructureBuffers DirectXUtil::AccelerationStructures::createPlaneBottomLevelAS(
	SampleFramework::ID3D12Device5Ptr pDevice, SampleFramework::ID3D12GraphicsCommandList4Ptr pCmdList)
{
	ShapeResources* planeData = createPrimitive(pDevice, PrimitiveType::QUAD);
	if (!planeData)
	{
		ThrowError("planeData is invalid");
	}

	//Create the AS
	return createBottomLevelAS(pDevice, pCmdList, planeData);
}

DirectXUtil::Structs::AccelerationStructureBuffers DirectXUtil::AccelerationStructures::createPrimitiveBottomLevelAS(
	SampleFramework::ID3D12Device5Ptr pDevice, SampleFramework::ID3D12GraphicsCommandList4Ptr pCmdList)
{
	//Delete if necessary (probably never)
	if (createdPrimitive)
	{
		delete createdPrimitive;
	}
	//Create the primitive
	createdPrimitive = createPrimitive(pDevice, PrimitiveType::SPHERE);

	if (!createdPrimitive)
	{
		ThrowError("createdPrimitive is invalid");
	}

	//Create the AS
	return createBottomLevelAS(pDevice, pCmdList, createdPrimitive);
}

void DirectXUtil::AccelerationStructures::buildTopLevelAS(SampleFramework::ID3D12Device5Ptr pDevice,
                                                          SampleFramework::ID3D12GraphicsCommandList4Ptr pCmdList,
                                                          std::vector<Structs::AccelerationStructureBuffers>
                                                          pBottomLevelAS,
                                                          uint64_t& tlasSize,
                                                          DirectXUtil::Structs::AccelerationStructureBuffers& buffers)
{
	static const int instances = 2;

	// First, get the size of the TLAS buffers and create them
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = instances;
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
		sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instances,
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		kUploadHeapProps
	);
	tlasSize = info.ResultDataMaxSizeInBytes;

	// Map the instance desc buffer
	D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs;
	buffers.pInstanceDesc->Map(0, nullptr, reinterpret_cast<void**>(&instanceDescs));
	ZeroMemory(instanceDescs, sizeof(D3D12_RAYTRACING_INSTANCE_DESC));

	// The transformation matrices for the instances
	glm::mat4 transformation[instances];
	transformation[0] = translate(glm::mat4(1.0f), glm::vec3(0, -1.0f, 0));
	transformation[1] = glm::mat4(1.0f);

	instanceDescs[0].InstanceID = 0; // This value will be exposed to the shader via InstanceID()
	instanceDescs[0].InstanceContributionToHitGroupIndex = 0;
	// This is the offset inside the shader-table. We only have a single geometry, so the offset 0
	instanceDescs[0].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	glm::mat3x4 mat = glm::mat3x4(glm::transpose(transformation[0]));
	memcpy(instanceDescs[0].Transform, &mat, sizeof(instanceDescs[0].Transform));
	instanceDescs[0].AccelerationStructure = pBottomLevelAS[0].pResult->GetGPUVirtualAddress();
	instanceDescs[0].InstanceMask = 0xFF;

	for (int i = 1; i < instances; i++)
	{
		instanceDescs[i].InstanceID = i;
		instanceDescs[i].InstanceContributionToHitGroupIndex = 0;
		instanceDescs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		mat = glm::mat3x4(glm::transpose(transformation[i]));
		memcpy(instanceDescs[i].Transform, &mat, sizeof(instanceDescs[i].Transform));
		instanceDescs[i].AccelerationStructure = pBottomLevelAS[i].pResult->GetGPUVirtualAddress();
		instanceDescs[i].InstanceMask = 0xFF;
	}

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
