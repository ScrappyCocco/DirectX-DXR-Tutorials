#include "AccelerationStructures.h"

#include <vector>
#include "../../../../_externals/glm/glm/vec3.hpp"
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

SampleFramework::ID3D12ResourcePtr DirectXUtil::AccelerationStructures::createTriangleVB(
	const SampleFramework::ID3D12Device5Ptr pDevice)
{
	const glm::vec3 vertices[] =
	{
		glm::vec3(0, 1, 0),
		glm::vec3(0.866f, -0.5f, 0),
		glm::vec3(-0.866f, -0.5f, 0),
	};

	// For simplicity, we create the vertex buffer on the upload heap, but that's not required
	SampleFramework::ID3D12ResourcePtr pBuffer = createBuffer(
		pDevice,
		sizeof(vertices),
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		kUploadHeapProps
	);

	uint8_t* pData;
	pBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData));
	memcpy(pData, vertices, sizeof(vertices));
	pBuffer->Unmap(0, nullptr);
	return pBuffer;
}

SampleFramework::ID3D12ResourcePtr DirectXUtil::AccelerationStructures::createPlaneVB(
	const SampleFramework::ID3D12Device5Ptr pDevice)
{
	const glm::vec3 vertices[] =
	{
		glm::vec3(-100, -1, -2),
		glm::vec3(100, -1, 100),
		glm::vec3(-100, -1, 100),

		glm::vec3(-100, -1, -2),
		glm::vec3(100, -1, -2),
		glm::vec3(100, -1, 100),
	};

	// For simplicity, we create the vertex buffer on the upload heap, but that's not required
	SampleFramework::ID3D12ResourcePtr pBuffer = createBuffer(
		pDevice,
		sizeof(vertices),
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		kUploadHeapProps
	);

	uint8_t* pData;
	pBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData));
	memcpy(pData, vertices, sizeof(vertices));
	pBuffer->Unmap(0, nullptr);
	return pBuffer;
}

DirectXUtil::Structs::AccelerationStructureBuffers DirectXUtil::AccelerationStructures::createBottomLevelAS(
	SampleFramework::ID3D12Device5Ptr pDevice, SampleFramework::ID3D12GraphicsCommandList4Ptr pCmdList,
	SampleFramework::ID3D12ResourcePtr pVB[], const uint32_t vertexCount[], const uint32_t geometryCount)
{
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDesc;
	geomDesc.resize(geometryCount);

	for (uint32_t i = 0; i < geometryCount; i++)
	{
		geomDesc[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geomDesc[i].Triangles.VertexBuffer.StartAddress = pVB[i]->GetGPUVirtualAddress();
		geomDesc[i].Triangles.VertexBuffer.StrideInBytes = sizeof(glm::vec3);
		geomDesc[i].Triangles.VertexCount = vertexCount[i];
		geomDesc[i].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geomDesc[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	}

	// Get the size requirements for the scratch and AS buffers
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = geometryCount;
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
		D3D12_RESOURCE_STATE_COMMON,
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
                                                          SampleFramework::ID3D12ResourcePtr pBottomLevelAS[2],
                                                          uint64_t& tlasSize, const float rotation, const bool update,
                                                          DirectXUtil::Structs::AccelerationStructureBuffers& buffers)
{
	// First, get the size of the TLAS buffers and create them
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	inputs.NumDescs = 3;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	if (update)
	{
		// If this a request for an update, then the TLAS was already used in a DispatchRay() call. We need a UAV barrier to make sure the read operation ends before updating the buffer
		D3D12_RESOURCE_BARRIER uavBarrier = {};
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = buffers.pResult.Get();
		pCmdList->ResourceBarrier(1, &uavBarrier);
	}
	else
	{
		// If this is not an update operation then we need to create the buffers, otherwise we will refit in-place
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
			sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * 3,
			D3D12_RESOURCE_FLAG_NONE,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			kUploadHeapProps
		);
		tlasSize = info.ResultDataMaxSizeInBytes;
	}

	// Map the instance desc buffer
	D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs;
	buffers.pInstanceDesc->Map(0, nullptr, reinterpret_cast<void**>(&instanceDescs));
	ZeroMemory(instanceDescs, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * 3);

	// The transformation matrices for the instances
	glm::mat4 transformation[3];
	const glm::mat4 rotationMat = glm::eulerAngleY(rotation);
	transformation[0] = glm::mat4(1.0);
	transformation[1] = translate(glm::mat4(1.0), glm::vec3(-2, 0, 0)) * rotationMat;
	transformation[2] = translate(glm::mat4(1.0), glm::vec3(2, 0, 0)) * rotationMat;

	// The InstanceContributionToHitGroupIndex is set based on the shader-table layout specified in createShaderTable()
	// Create the desc for the triangle/plane instance
	instanceDescs[0].InstanceID = 0;
	instanceDescs[0].InstanceContributionToHitGroupIndex = 0;
	instanceDescs[0].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	memcpy(instanceDescs[0].Transform, &transformation[0], sizeof(instanceDescs[0].Transform));
	instanceDescs[0].AccelerationStructure = pBottomLevelAS[0]->GetGPUVirtualAddress();
	instanceDescs[0].InstanceMask = 0xFF;

	for (uint32_t i = 1; i < 3; i++)
	{
		instanceDescs[i].InstanceID = i; // This value will be exposed to the shader via InstanceID()
		instanceDescs[i].InstanceContributionToHitGroupIndex = (i * 2) + 2;
		// The indices are relative to to the start of the hit-table entries specified in Raytrace(), so we need 4 and 6
		instanceDescs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		glm::mat4 m = glm::transpose(transformation[i]); // GLM is column major, the INSTANCE_DESC is row major
		memcpy(instanceDescs[i].Transform, &m, sizeof(instanceDescs[i].Transform));
		instanceDescs[i].AccelerationStructure = pBottomLevelAS[1]->GetGPUVirtualAddress();
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

	// If this is an update operation, set the source buffer and the perform_update flag
	if (update)
	{
		asDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
		asDesc.SourceAccelerationStructureData = buffers.pResult->GetGPUVirtualAddress();
	}

	pCmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = buffers.pResult.Get();
	pCmdList->ResourceBarrier(1, &uavBarrier);
}
