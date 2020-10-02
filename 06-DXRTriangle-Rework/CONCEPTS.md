# [Nvidia] DXR "HelloTriangle"  - Important Concepts
This document is a summarize and a rework of the concepts of the Nvidia tutorial, in order to focus attention on some parts and functions in the code. Take the original tutorial as a reference.

Thanks Nvidia and its [repository contributors](https://github.com/NVIDIAGameWorks/DxrTutorials/graphs/contributors) for this tutorial, all this work is theirs.

Note that the whole tutorial documentation in the Nvidia repository is in .doc/.docx files, so you need to download them or clone the repository in order to open and read them.

## Contents
- [Introduction](#introduction)
- [Smart Pointers in this tutorial](#smart-pointers-in-this-tutorial)
- [GLM `#define`](#glm-define)
- [Window creation](#window-creation)
  - [Events](#events)
- [Initialization](#initialization)
- [Accelleration structure](#accelleration-structure)
- [RT Pipeline](#rt-pipeline)
  - [Shader libraries](#shader-libraries)
  - [Shader code](#shader-code)
  - [Creating the RT Pipeline State Object](#creating-the-rt-pipeline-state-object)
    - [DxilLibrary](#dxillibrary)
    - [HitProgram](#hitprogram)
    - [LocalRootSignature](#localrootsignature)
    - [ExportAssociation](#exportassociation)
    - [ShaderConfig](#shaderconfig)
    - [PipelineConfig](#pipelineconfig)
    - [GlobalRootSignature](#globalrootsignature)
    - [Creation](#creation)
- [Shader Table](#shader-table)
  - [Nvidia suggestion](#nvidia-suggestion)
  - [Basic description](#basic-description)
  - [Shader Table Records](#shader-table-records)
  - [Creation](#creation-1)
- [Raytracing](#raytracing)
- [Further reading](#further-reading)
  - [Details](#details)
  - [Original tutorial](#original-tutorial)

# Introduction
This document require that you understood the basic Raytracing flow made in Tutorial 5, so know the basics to render multiple Raytraced triangles, and can go in-depth to see how to render the same example without using helper classes.

The basics are exactly the same of the past 5 tutorials, this one is only larger because explicitly declare and allocate lots of resources that were in the helper classes.

# Smart Pointers in this tutorial
Nvidia in its code use a small different type of smart pointer, defined in Framework.h as:
```cpp
#define MAKE_SMART_COM_PTR(_a) _COM_SMARTPTR_TYPEDEF(_a, __uuidof(_a))
MAKE_SMART_COM_PTR(ID3D12Device5); //Example of usage
```
I decided to keep using Microsoft one:
```cpp
Microsoft::WRL::ComPtr
```
as it was in older tutorials, so all the code has been updated to use this type of smart pointers.

For simplicity, most of these smart pointers used in the project are defined in the file [InterfacePointers.h](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/Dx12/InterfacePointers.h).

# GLM `#define`
This tutorial use glm for matrices and other mathematical operations.

To make sure glm is used as intended, this tutorial **doesn't define:**
```
#define GLM_FORCE_CTOR_INIT
#define GLM_ENABLE_EXPERIMENTAL
```
So every initialization is done manually or writing values in the constructor.

# Window creation
The window management is different from Nvidia code but equal to other tutorials, so you can check [Win32Application.cpp](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/Win32Application.cpp) about that.

## Events
Once again, this tutorial use the virtual calls:
```cpp
virtual void OnInit();
virtual void OnUpdate();
virtual void OnRender();
virtual void OnDestroy();
```
defined in [DXSample.h#L12](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/DXSample.h#L12).

# Initialization
We will need to create a device, swap-chain, command-queue, command-list, command-allocator, descriptor-heap and a fence. Remember – it is assumed that the user is familiar with DirectX12 programming, so we will not actually cover most of those objects.

Note that DXR functions are a part of the ID3D12Device5 and ID3D12GraphicsCommandList4 interfaces.

All the DXR resource creation is in file [D3D12GraphicsContext.cpp](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/Utils/D3D12GraphicsContext.cpp).

The Initialization of the whole project is made in [D3D12HelloTriangle.cpp#L24](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/D3D12HelloTriangle.cpp#L24).

# Accelleration structure
Once again:
* An acceleration structure is an opaque data structure that represents the scene’s geometry. This structure is used in rendering time to intersect rays against;
* The BLAS is a data structure that represent a local-space mesh. It does not contain information regarding the world-space location of the vertices or instancing information;
* The TLAS is an opaque data structure that represents the entire scene. As you recall, BLAS represents objects in local space. The TLAS references the bottom-level structures, with each reference containing local-to-world transformation matrix.

It's managed inside [AccelerationStructures.cpp](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/Utils/AccelerationStructures.cpp).

The creation is made from [D3D12HelloTriangle.cpp#L248](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/D3D12HelloTriangle.cpp#L248).
And as you can see the order of creation events is quite clear.

Note that the triangle rotation this time is not inside `OnUpdate()` but inside `buildTopLevelAS(...)` -> [AccelerationStructures.cpp#L232](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/Utils/AccelerationStructures.cpp#L232).

The basics seen in tutorial 5 about updating the TopLevelAS remain the same.

After creating some buffers and recorded commands to create bottom-level and top-level acceleration structures, now we need to execute the command-list. To simplify resource lifetime management, we will submit the list and wait until the GPU finishes its execution. This is not required by the spec – the list can be submitted whenever as long as the resources are kept alive until execution finishes.

The last part is releasing resources that are no longer required and keep references to the resources which will be used for rendering.

# RT Pipeline
The RT Pipeline is managed inside [RTPipeline.cpp](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/Utils/RTPipeline.cpp).

## Shader libraries
dxcompiler, the new SM6.x compiler, introduces a new concept called shader-libraries. Libraries allow us to compile a file containing multiple shaders without specifying an entry point. We create shader libraries by specifying "lib_6_3" as the target profile, which requires us to use an empty string for the entry point.

Using dxcompiler is straightforward but is not in the scope of this tutorial.

You can see how is used here [RTPipeline.cpp#L29](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/Utils/RTPipeline.cpp#L29).

## Shader code
The [Shader code](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/shaders/Shaders.hlsl) is basically the same of tutorial 5, except this time is all inside a single file.

## Creating the RT Pipeline State Object
This use some abstractions to make the creation easier:
### [DxilLibrary](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/Utils/Structs/DxilLibrary.h)
This is our abstraction for a sub-object of type `D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY`.

The library accepts a single ID3DBlob object which contains an SM6.1 library. This library can contain multiple entry points, and we need to specify which entry points we plan to use. In our case, we store shaders entry points.

2 things to note:
1. We cache the entry-point name into a pre-allocated member vector of strings;
2. We set `ExportToRename` to `nullptr`. Later we will see that we need a way to identify each shader inside a state-object. This is usually done by passing the entry-point name to the required function. There could be cases where shaders from different blobs share the same entry-point name, making the identification ambiguous. To resolve this, we can use `ExportToRename` to give each shader a unique name. In our case, we set it to `nullptr` since each shader has a unique-entry point name.

### [HitProgram](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/Utils/Structs/HitProgram.h)
HitProgram is an abstraction over a `D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP` sub-object. A hit-group is a collection of intersection, any-hit and closest-hit shaders, at most one of each type. Since we don’t use custom intersection-shaders in these tutorials, our HitProgram object only accepts AHS and CHS entry point name.

### [LocalRootSignature](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/Utils/Structs/LocalRootSignature.h)
DXR introduces a new concept called Local Root Signature (LRS). In graphics and compute pipelines, we have a single, global root-signature used by all programs. For ray-tracing, in addition to that root-signature, we can create local root-signatures and bind them to specific shaders.

### [ExportAssociation](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/Utils/Structs/ExportAssociation.h)
An ExportAssociation object binds a sub-object into shaders and hit-groups.

### [ShaderConfig](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/Utils/Structs/ShaderConfig.h)
Next bit is the shader configuration. There are 2 values we need to set:
1. The payload size in bytes. This is the size of the payload struct we defined in the HLSL. In our case the payload is a single bool (4-bytes in HLSL);
2. The attributes size in bytes. This is the size of the data the hit-shader accepts as its intersection-attributes parameter. For the built-in intersection shader, the attributes size is 8-bytes (2 floats).

### [PipelineConfig](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/Utils/Structs/PipelineConfig.h)
The pipeline configuration is a global sub-object affecting all pipeline stages. In the case of raytracing, it contains a single value - MaxTraceRecursionDepth. This value simply tells the pipeline how many recursive raytracing calls we are going to make.

### [GlobalRootSignature](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/Utils/Structs/GlobalRootSignature.h)
The last piece of the puzzle is the global root-signature. As the name suggests, this root-signature affects all shaders attached to the pipeline. The final root-signature of a shader is defined by both the global and the shader’s local root-signature. The code is straightforward.

### Creation
With all this structures, we can initialize all the necessary data and finally craete the pipeline object: [D3D12HelloTriangle.cpp#L285](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/D3D12HelloTriangle.cpp#L285).

# Shader Table
## Nvidia suggestion
I strongly urge you to read the Shader-Table section [in the spec](https://github.com/microsoft/DirectX-Specs/blob/master/d3d/Raytracing.md#shader-tables). It covers many topics and details which are far beyond the scope of this tutorial. This tutorial will only cover the basic concepts. In the next tutorials we will see more advanced usages of the Shader-Table.

## Basic description
The last piece required for rendering is the Shader-Table. It’s a GPU-visible buffer which is owned and managed by the application – allocation, data updates, etc.

The shader-table is an array of records and it has 2 roles:
1. Describe the relation between the scene’s geometry and the program to be executed;
2. Bind resources to the pipeline.
The first role is required because we can have multiple hit and miss programs attached to the state object and we need to know which shader to execute when a geometry is hit (or nothing was hit).

The second role is required because:
1. We can create each program with a different local root-signature;
2. Each geometry might require a different set of resources (vertex-buffer, textures, etc.)
Note that the API allows to use multiple shader-tables in a single DispatchRays() call. For simplicity, we will use a single shader-table in this tutorial.

## Shader Table Records
Each shader-table record has 2 sections. It begins with an opaque program identifier (obtained by calling `ID3D12StateObjectPropertiesPtr::GetShaderIdentifier()` followed by a root table containing the shader resource bindings.

The root-table is very similar to the regular rasterization root-table. The difference is that in our case we set the entries directly into the buffer instead of using setter methods. 

The sizes of the different entries are slightly different than those described in the D3D12 root signature limits:
* Root Constants are 4 bytes;
* Root Descriptors are 8 bytes;
* Descriptor Tables are 8 bytes. This is different than the size required by the regular root-signature.

For root constants and root descriptors we set the same data as what would be passed to the setter functions. Descriptor table is different – we need to set the `D3D12_GPU_DESCRIPTOR_HANDLE::ptr` field.

Another important thing is that root-descriptors must be stored at an 8-byte aligned address, so in some cases padding might be required.

## Creation
You can check the Shader Table creation in this method here: [D3D12HelloTriangle.cpp#L403](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/D3D12HelloTriangle.cpp#L403).

# Raytracing
Now that everything is in place we can update the TopLevelAS with the new rotations, and bind the data to finally call the `DispatchRays`: [D3D12HelloTriangle.cpp#L116](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/6-DXRTriangle-Rework/Project/Source/D3D12HelloTriangle.cpp#L116).

# Further reading
## Details
The basics about topics such as PerInstanceConstantBuffer/PerGeometryHitShader are the same discussed in the old tutorials, the only difference is that without helper classes there is more code to do it from "scratch". So if you want to read more about them consider reading the Nvidia .docx about that topic in the original tutorial folder.

## Original tutorial
You can check [NVIDIAGameWorks/DxrTutorials](https://github.com/NVIDIAGameWorks/DxrTutorials) if you want to follow the tutorial yourself or check Nvidia original code.
