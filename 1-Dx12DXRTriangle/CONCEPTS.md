# [Nvidia] DXR "HelloTriangle"  - Important Concepts
This document summarize most of the concepts of the Nvidia tutorial, in order to focus attention on some parts and functions in the code. Note that this document will omit some details about a few variables types and calls that are not strictly necessary to understand the flow.

Again, huge thanks to [Martin-Karl Lefrançois](https://devblogs.nvidia.com/author/mlefrancois/) and [Pascal Gautron](https://devblogs.nvidia.com/author/pgautron/) that made this tutorial, all this work is theirs, i just copied and changed it a bit for educational purposes.

## Contents
* [Introduction](#Introduction)

# Introduction
As Nvidia says, this is only for educational purposes to showcase a basic integration of raytracing, a real integration would require additional levels of abstraction.

# Checking support
The application need to check if the device (the GPU) support Raytracing, this is done checking:
```cpp
RaytracingTier >= D3D12_RAYTRACING_TIER_1_0
```
If the device `RaytracingTier` is < that `D3D12_RAYTRACING_TIER_1_0`, then we can't use it for Raytracing. 

You can see that check here in the code: [D3D12HelloTriangle.cpp#L494](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/1-Dx12DXRTriangle/Project/D3D12HelloTriangle.cpp#L494)

## Dx12 Feature Level
This is not a must, but for consistency and to show that you use Dx12, is better to create the device with the flag `D3D_FEATURE_LEVEL_12_1` so you sare saying that your application will targets features supported by Direct3D 12.1, including shader model 5. You can read more about feature levels in the [Microsoft Documentation](https://docs.microsoft.com/en-us/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_feature_level). 

You can see it here in the code: [D3D12HelloTriangle.cpp#L96](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/1-Dx12DXRTriangle/Project/D3D12HelloTriangle.cpp#L96) where the device is created.

# Acceleration Structures
Raytracing using DXR requires 3 main building blocks:
1. The **acceleration structures** store the geometric information to optimize the search for ray intersection points, and are separated into the bottom-level (BLAS) and top-level (TLAS);
2. The **raytracing pipeline** contains the compiled programs along with the function names associated to each shader program, as well as a description of how those shaders exchange data;
3. The **shader binding table** is linking all of those pieces together by indicating which shader programs are executed for which geometry, and which resources are associated with it.

As you might have seen in the Readme, in this tutorial we're using [Nvidia DX12 Raytracing tutorial Helpers Classes](https://developer.nvidia.com/rtx/raytracing/dxr/DX12-Raytracing-tutorial/dxr_tutorial_helpers) to facilitate the first contact with DXR.

## Description of Acceleration Structures
To be efficient, raytracing requires putting the geometry in an acceleration structure (AS) that will reduce the number of ray-triangle intersection tests during rendering. Those BLAS then hold the actual vertex data of each object. 

However, it is also possible to combine multiple objects within a single bottom-level AS: for that, a single BLAS can be built from multiple vertex buffers, each with its own transform matrix. Note that if an object is instantiated several times within a same BLAS, its geometry will be duplicated (as a rule of thumb, the fewer BLAS, the better).

## Building the Acceleration Structures
Building an acceleration structure (AS) requires up to 3 buffers:
1. Some scratch memory used internally by the acceleration structure builder;
2. The actual storage of the structure;
3. The descriptors representing the instance matrices for top-level acceleration structures.

This tutorial will only use a single bottom-level AS, for which we only store the `pResult` buffer, that you can see here in the code: [D3D12HelloTriangle.h#L87](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/1-Dx12DXRTriangle/Project/D3D12HelloTriangle.h#L87).

### (1) `CreateBottomLevelAS(vVertexBuffers)`:
This method generates the bottom-level AS from a vector of vertex buffers in GPU memory and vertex count.

We're passing this method an array of two elements (pair):
```cpp
std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers
``` 
The first element is the pointer to the resource holding the vertices of the geometry, the second is the number of vertices. Note that we are assuming that the resource contains Vertex structures. For the sake of simplicity, we do not use indexing: triangles are described by 'Vertex' triplets.

The BLAS generation is performed on GPU, and the second step computes the storage requirements to hold the final BLAS as well as some temporary space by calling **ComputeASBufferSizes**, present in the helper class BottomLevelASGenerator. This maps to the actual DXR API, which requires the application to allocate the space for the BLAS as well as the temporary (scratch) space. This scratch space can be freed as soon as the build is complete.

Finally, the BLAS can be generated by calling the **Generate** method. It will create a descriptor of the acceleration structure building work, and call `ID3D12GraphicsCommandList4::BuildRaytracingAccelerationStructure()` with that descriptor.

The resulting BLAS contains the full definition of the geometry, organized in a way suitable for efficiently finding ray intersections with that geometry.

### (2) `CreateTopLevelAS(instances)`:
This method generates the top-level AS from a vector of bottom-level AS and transform matrix.

The top level acceleration structure (TLAS) can be seen as an acceleration structure over acceleration structures, which aims at optimizing the search for ray intersections in any of the underlying BLAS. A TLAS can instantiate the same BLAS multiple times, using per-instance matrices to render them at various world-space positions.

We're passing this method an array of two elements (pair):
```cpp
const std::vector<std::pair<comptr<id3d12resource>, DirectX::XMMATRIX>>& instances
```
The first element is the resource pointer to the BLAS, the second is the matrix to position the object.

This method is very similar in structure to CreateBottomLevelAS, with the same 3 steps:
1. Gathering the input data;
2. computing the AS buffer sizes;
3. generating the actual TLAS.

However, the TLAS requires an additional buffer holding the descriptions of each instance. The **ComputeASBufferSizes** method provides the sizes of the scratch and result buffers, and computes the size of the instance buffers from the size of the instance descriptor and the number of instances.

As for the BLAS, the scratch and result buffers are directly allocated in GPU memory, on the default heap. The instance descriptors buffer will need to be mapped within the helper, and has to be allocated **on the upload heap**.

Once the buffers are allocated, the **Generate** call fills in the instance descriptions buffer and a descriptor of the building work to be done. This descriptor is then passed to `ID3D12GraphicsCommandList4::BuildRaytracingAccelerationStructure` which builds an acceleration structure holding all the instances.

### (3) `CreateAccelerationStructures()`:
This method binds the above methods together, and store the generated structures. Note that while we only keep resulting BLAS of the triangle and discard the scratch space, we store all buffers for the TLAS into `m_topLevelASBuffers` in anticipation of the handling of dynamic scenes, where the scratch space will be used repeatedly.

This method first fills the command list with the build orders for the bottom-level acceleration structures. For each BLAS, the helper introduces a resource barrier `D3D12_RESOURCE_BARRIER_TYPE_UAV` to ensure the BLAS can be queried within the same command list. This is required as the top-level AS is also built in that command list. After enqueuing the AS build calls, we execute the command list immediately by calling **ExecuteCommandLists** and using a fence to flush the command list before starting rendering.

# The Pipeline
In a raytracing context, a ray traced to the scene can hit any object and thus trigger the execution of any shader. Instead of using one shader executable at a time, we now need to have all shaders available at once. The pipeline then contains all the shader required to render the scene, and information on how to execute it. To be able to raytrace some geometry, a DXR pipeline typically uses at least those 3 HLSL shader program types:

* **The ray generation** program, that will be the starting point of the raytracing, and called for each pixel: it will typically initialize a ray descriptor starting at the location of the camera, in a direction given by evaluating the camera lens model at the pixel location. It will then invoke **TraceRay()**, that will shoot the ray in the scene. Other shaders below will process further events, and return their result to the ray generation shader through the ray payload;
* The **miss shader** is executed when a ray does not intersect any geometry. It can typically sample an environment map, or return a simple color through the ray payload;
* The **closest hit shader** is called upon hitting a the geometric instance closest to the starting point of the ray. This shader can for example perform lighting calculations, and return the results through the ray payload. There can be as many closest hit shaders as needed, in the same spirit as a rasterization-based application has multiple pixel shaders depending on the objects.

Two more shader types can optionally be used:

* The intersection shader, which allows intersecting user-defined geometry. Using this shader requires modifying how the acceleration structures are built, and is beyond the scope of this tutorial. We will instead rely on the built-in triangle intersection shader provided by DXR;
* The any hit shader is executed on each potential intersection: when searching for the hit point closest to the ray origin, several candidates may be found on the way. The any hit shader can typically be used to efficiently implement alpha-testing. If the alpha test fails, the ray traversal can continue without having to call **TraceRay()** again.

## Shaders
In this tutorial we will create a pipeline containing only the 3 mandatory shader programs: a single ray generation, single miss and a single closest hit.
Shaders used:

* [Common.hlsl](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/1-Dx12DXRTriangle/Project/shaders/Common.hlsl): is included by the other shaders, and defines the ray payload which will be used to communicate information between shaders. It only contains a `float4` vector representing the color at the hit point and the distance from the ray origin to that hit point. This file also declares the structure which will be used to store the `float2` barycentric coordinates returned by the intersection shader;
* [RayGen.hlsl](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/1-Dx12DXRTriangle/Project/shaders/RayGen.hlsl): contains the ray generation program **RayGen()**. It also declares its access to the raytracing output buffer gOutput bound as a unordered access view (UAV), and the raytracing acceleration structure SceneBVH, bound as a shader resource view (SRV);
* [Miss.hlsl](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/1-Dx12DXRTriangle/Project/shaders/Miss.hlsl): defines the **Miss()** shader. This shader will be executed when no geometry is hit, and will write a constant color in the payload. Note that this shader takes the payload as a inout parameter. It will be provided to the shader automatically by DXR;
* [Hit.hlsl](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/1-Dx12DXRTriangle/Project/shaders/Hit.hlsl): contains a very simple closest hit shader **ClosestHit()**. It will be executed upon hitting the geometry (our triangle). As the miss shader, it takes the ray payload payload as a inout parameter. It also has a second parameter defining the intersection attributes as provided by the intersection shader, ie. the barycentric coordinates.

## Shaders bindings
Shaders usually require external data, such as textures, constant buffers etc. Those inputs are specified in the shader code, by binding data to a given register:
```hlsl
RWTexture2D<float4> gOutput : register(u0);
``` 
In this case, the register means the data is accessible in the first unordered access variable (UAV, identified by the letter `u`) bound to the shader. Constant buffers (CBV), can be accessed using the letter `b`. Shader resources (SRV) correspond to the letter `t`.

The binding of the resources is also defined explicitly using root signatures, which must match the order and types defined within the shader. Each binding in ther shader corresponds to an entry in the root signature. A root signature entry is defined by a `D3D12_ROOT_PARAMETER`.

### `CreateRayGenSignature()`:
The root signature of the RayGen program indicates the program needs to access the image output and the buffer containing the Top Level Acceleration Structure. For the sake of simplicity the root signatures introduced in this tutorial use our `RootSignatureGenerator` helper.

# Raytracing Pipeline
The raytracing pipeline binds the shader code, root signatures and pipeline characteristics in a single structure used by DXR to invoke the shaders and manage temporary memory during raytracing. The setup of the pipeline requires creating and combining numerous DirectX 12 subobjects, a concept introduced with DXR. In this tutorial we use the `RayTracingPipeline` helper to simplify the code and enforce consistency across the created objects.

We start the method by generating the DXIL libraries for each shader, calling `nv_helpers_dx12::CompileShaderLibrary`.
A DXIL library can be seen similarly as a regular DLL, which contains compiled code that can be accessed using a number of exported symbols. In the case of the raytracing pipeline, such symbols correspond to the names of the functions implementing the shader programs. For each file, we then add the library pointer in the pipeline, along with the name of the function it contains. Note that a single library can export an arbitrary number of symbols, and that the pipeline is allowed to only import a subset of those.

To be used, each shader **needs to be associated to its root signature**. A shaders imported from the DXIL libraries needs to be associated with exactly one root signature. The shaders comprising the hit groups need to share the same root signature, which is associated to the hit group (and not to the shaders themselves). Note that a shader does not have to actually access all the resources declared in its root signature, as long as the root signature defines a superset of the resources the shader needs.

The Set* methods setup specific subobjects related to the global properties of the pipeline itself:

* The **maximum payload size**: used by the shaders: which defines the amount of data a shader can exchange with other shaders. A typical example is a payload containing a color value, so that the hit or miss shaders can return this value to the ray generation program, which will write that value to the output buffer. To achieve best performance the payload needs to be kept as small as possible. In our case the payload contains 4 floating-point values stored in the **HitInfo** structure in Common.hlsl, representing the output color and the distance of the hit from the ray origin.
* The **attributes size**: which is set by the intersection shader. We use the built-in triangle intersection shader that return 2 floating-point values corresponding to the barycentric coordinates of the hit point inside the triangle. Those values are accessed using the **Attributes** structure in Common.hlsl
* The **maximum recursion depth**: which describes how many nested **TraceRay()** calls can be made while tracing. One level of recursion can for example be used to shoot shadow rays from a hit point. Note that, however, this recursion level must be kept as low as possible for performance. Therefore, implementing a path tracer should not be done using recursion: the multiple bounces should be implemented in a loop in the ray generation program instead. In our case, we will only call **TraceRay() once** to cast a ray from the camera without further bounces, hence a recursion level of 1.

The pipeline now has all the information it needs. We generate the pipeline by calling the `Generate` method of the helper.

# Resources
Unlike the rasterization, the raytracing process does not write directly to the render target: instead, it writes its results into a buffer bound as an unordered access view (UAV), which is then copied to the render target for display. Also, any shader program that calls TraceRay() needs to be able to access the top-level acceleration structure (TLAS).

In this section we will first create the raytracing output buffer `m_outputResource`, and then create the heap `m_srvUavHeap` referencing both that buffer and the TLAS.

### `CreateRaytracingOutputBuffer()`:
The method below allocates the **buffer holding the raytracing output** using `ID3D12Device::CreateCommittedResource`, with the same size as the output image. This buffer is initialized in the copy source state `D3D12_RESOURCE_STATE_COPY_SOURCE`, which is the state assumed by the PopulateCommandList method. That method will transition the buffer to a `D3D12_RESOURCE_STATE_UNORDERED_ACCESS`, perform raytracing and transition back to `D3D12_RESOURCE_STATE_COPY_SOURCE` so that the contents of the buffer can be copied to the render target using `ID3D12GraphicsCommandList::CopyResource`. It is then important that the raytracing output buffer is created with the `D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS` flag.

### `CreateShaderResourceHeap()`:
The data accessible to all shaders is typically referenced in a heap bound before rendering. This heap contains a predefined number of slots, each of them containing a view on an object in GPU memory. In practice, the heap is a memory area containing views on common resources.

In this tutorial the heap only contains two entries: the raytracing output buffer accessed as a UAV, and the top-level acceleration structure which is a shader resource (SRV) with a specific dimension flag `D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE`.

# The Shader Binding Table