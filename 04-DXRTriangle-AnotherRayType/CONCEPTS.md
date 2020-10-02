# [Nvidia] DXR "HelloTriangle"  - Important Concepts
This document is a summarize and a rework of the concepts of the Nvidia tutorial, in order to focus attention on some parts and functions in the code.

Again, huge thanks to [Martin-Karl Lefrançois](https://devblogs.nvidia.com/author/mlefrancois/) and [Pascal Gautron](https://devblogs.nvidia.com/author/pgautron/) that made this tutorial, all this work is theirs, i just took it to change and summarize it a bit for educational purposes.

## Contents
- [Introduction](#introduction)
- [Editing Shaders](#editing-shaders)
  - [Payload info](#payload-info)
  - [Shadow Shader](#shadow-shader)
  - [Hit shader](#hit-shader)
- [Changing the HitSignature](#changing-the-hitsignature)
- [Changing the RaytracingPipeline](#changing-the-raytracingpipeline)
- [Changing the ShaderBindingTable](#changing-the-shaderbindingtable)
  - [Miss](#miss)
  - [Hit](#hit)
  - [Changing Plane Hit](#changing-plane-hit)
- [Changing the TopLevelAS](#changing-the-toplevelas)
- [Further reading](#further-reading)

# Introduction
This document require that you understood the basic Raytracing flow made in Tutorial 3, so you know the basics about how to add geometry and rays for different geometry so we can add basic shadows.

# Editing Shaders
## Payload info
We're adding a payload for the new ray called `ShadowHitInfo`.

When hitting a surface the payload is set to true, while when missing all geometry the ShadowMiss is invoked, setting the payload to false.

You can see it here: [Common.hlsl#L16](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/4-DXRTriangle-AnotherRayType/Project/shaders/Common.hlsl#L16).

## Shadow Shader
Now, with the new Payload, we can create the Shader: [ShadowRay.hlsl](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/4-DXRTriangle-AnotherRayType/Project/shaders/ShadowRay.hlsl).

## Hit shader
The hit shader needs to be able to cast shadow rays. To cast more rays, the hit shader for the plane needs to access the top-level acceleration structure, we're adding it here: [Hit.hlsl#L18](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/4-DXRTriangle-AnotherRayType/Project/shaders/Hit.hlsl#L18).

Now the `PlaneClosestHit` function can then be modified to shoot shadow rays. From the hit point we initialize a shadow ray towards a hardcoded light position `lightPos`.

The payload after the trace call indicates whether a surface has been hit, and we use it to modify the output color.

You can see the whole function here: [Hit.hlsl#L35](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/4-DXRTriangle-AnotherRayType/Project/shaders/Hit.hlsl#L35).

# Changing the HitSignature
Since the hit shader now needs to access the scene data, its root signature needs to be enhanced to get access to the SRV containing the top-level acceleration structure, which is stored in the second slot of the heap.

You can see the changed HitSignature here: [D3D12HelloTriangle.cpp#L719](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/4-DXRTriangle-AnotherRayType/Project/D3D12HelloTriangle.cpp#L719).

# Changing the RaytracingPipeline
Now need to load the shader library and export the corresponding symbols:

* [D3D12HelloTriangle.cpp#L763](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/4-DXRTriangle-AnotherRayType/Project/D3D12HelloTriangle.cpp#L763): First load and compile the new library;
* [D3D12HelloTriangle.cpp#L774](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/4-DXRTriangle-AnotherRayType/Project/D3D12HelloTriangle.cpp#L774): Export the symbols;
* [D3D12HelloTriangle.cpp#L783](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/4-DXRTriangle-AnotherRayType/Project/D3D12HelloTriangle.cpp#L783): Create the HitSignature;
* [D3D12HelloTriangle.cpp#L805](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/4-DXRTriangle-AnotherRayType/Project/D3D12HelloTriangle.cpp#L805): Add a new HitGroup;
* [D3D12HelloTriangle.cpp#L819](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/4-DXRTriangle-AnotherRayType/Project/D3D12HelloTriangle.cpp#L819): Associate the group with its RootSignature;
* [D3D12HelloTriangle.cpp#L815](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/4-DXRTriangle-AnotherRayType/Project/D3D12HelloTriangle.cpp#L815): The miss signature is the same so we can add the `ShadowMiss` with the old Miss signature;
* [D3D12HelloTriangle.cpp#L840](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/4-DXRTriangle-AnotherRayType/Project/D3D12HelloTriangle.cpp#L840): Since it will now be possible to shoot rays from a hit point, this means rays are traced recursively. We then increase the allowed recursion level to 2.

# Changing the ShaderBindingTable
The raytracing pipeline is ready to shoot shadow rays, but the actual shader still needs to be associated to the geometry in the Shader Binding Table.

## Miss
We can add the shadow miss program after the original miss: [D3D12HelloTriangle.cpp#L962](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/4-DXRTriangle-AnotherRayType/Project/D3D12HelloTriangle.cpp#L962).

## Hit
The shadow hit group is added right after **adding each addition** of the original hit group, so that all the geometry can be hit:
* After each triangle: [D3D12HelloTriangle.cpp#L975](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/4-DXRTriangle-AnotherRayType/Project/D3D12HelloTriangle.cpp#L975);
* After the plane: [D3D12HelloTriangle.cpp#L982](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/4-DXRTriangle-AnotherRayType/Project/D3D12HelloTriangle.cpp#L982).

## Changing Plane Hit
The resources for the plane hit group need to be enhanced to give access to the heap: [D3D12HelloTriangle.cpp#L979](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/4-DXRTriangle-AnotherRayType/Project/D3D12HelloTriangle.cpp#L979).

# Changing the TopLevelAS
In the previous tutorials we indicated that the hit group index of an instance is equal to its instance index, since we only had one hit group per instance. Now we have two hit groups (primary and shadow), so the hit group index has to be `2*i`, where `i` is the instance index: [D3D12HelloTriangle.cpp#L578](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/4-DXRTriangle-AnotherRayType/Project/D3D12HelloTriangle.cpp#L578).

# Further reading
Remember to check [DXR Resources](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials#resources) if you want to know more about the topic or about how to realize these tutorials by yourself.
