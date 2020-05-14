# [Nvidia] DXR "HelloTriangle"  - Important Concepts
This document is a summarize and a rework of the concepts of the Nvidia tutorial, in order to focus attention on some parts and functions in the code.

Again, huge thanks to [Martin-Karl Lefrançois](https://devblogs.nvidia.com/author/mlefrancois/) and [Pascal Gautron](https://devblogs.nvidia.com/author/pgautron/) that made this tutorial, all this work is theirs, i just took it to change and summarize it a bit for educational purposes.

## Contents
- [Introduction](#introduction)
- [Note](#note)
- [Adding more Triangles](#adding-more-triangles)
- [Adding a Plane](#adding-a-plane)
  - [Updating the AccelerationStructures](#updating-the-accelerationstructures)
- [Creating Per-Instance Constant Buffer](#creating-per-instance-constant-buffer)
- [Changing the ShaderBindingTable](#changing-the-shaderbindingtable)
- [Changing the TopLevelAS](#changing-the-toplevelas)
- [Changing the HitSignature](#changing-the-hitsignature)
- [Adding a Specific Hit Shader for the Plane](#adding-a-specific-hit-shader-for-the-plane)
  - [Changing Hit.hlsl](#changing-hithlsl)
  - [Changing the RaytracingPipeline](#changing-the-raytracingpipeline)
  - [Changing the ShaderBindingTable](#changing-the-shaderbindingtable-1)
- [Further reading](#further-reading)

# Introduction
This document require that you understood the basic Raytracing flow made in Tutorial 2, so you know the basics about how to use a perspective camera and you can start adding more geometry.

# Note
This tutorial, that you can find on Nvidia website, is full of unspecified things and jumps between one topics such using a Global Constant Buffer and then using a Per-Instance Constant Buffer. Since the code you can find in my Repository is the "final" result I'll directly explain how to use a Per-Instance Constant Buffer when adding Geometry. If you want to know more consider reading the full tutorial along with this final code.

# Adding more Triangles
Adding more instances of the triangle is fairly simple, as it only requires adding those in the top-level acceleration structure.

Look here at `CreateAccelerationStructures`: [D3D12HelloTriangle.cpp#L641](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/3-DXRTriangle-PerInstanceData/Project/D3D12HelloTriangle.cpp#L641).

# Adding a Plane
We need to create the vertex buffer for the plane, as you can see here: [D3D12HelloTriangle.cpp#L1051](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/3-DXRTriangle-PerInstanceData/Project/D3D12HelloTriangle.cpp#L1051).

## Updating the AccelerationStructures
Now that we have the plane buffers we can create the BLAS of the plane: [D3D12HelloTriangle.cpp#L636](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/3-DXRTriangle-PerInstanceData/Project/D3D12HelloTriangle.cpp#L636).

And then add a reference to the bottom-level AS of the plane in the instance list (you may have noticed it already): [D3D12HelloTriangle.cpp#L646](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/3-DXRTriangle-PerInstanceData/Project/D3D12HelloTriangle.cpp#L646).

# Creating Per-Instance Constant Buffer
In most practical cases, the constant buffers are defined per-instance so that they can be managed independently.

We create one buffer for each triangle, as you can see here: [D3D12HelloTriangle.cpp#L1100](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/3-DXRTriangle-PerInstanceData/Project/D3D12HelloTriangle.cpp#L1100).

# Changing the ShaderBindingTable
The hit groups for each instance and the actual pointers to the constant buffer are then set in the Shader Binding Table. We will add a hit group for each triangle, and one for the plane, so that each can point to its own constant buffer: [D3D12HelloTriangle.cpp#L946](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/3-DXRTriangle-PerInstanceData/Project/D3D12HelloTriangle.cpp#L946).

# Changing the TopLevelAS
Now the instances are independent, we need to associate the instances with their own hit group in the SBT.

We need to indicate that the instance index i is also the index of the hit group to use in the SBT. This way, hitting the i-th triangle will invoke the first hit group defined in the SBT: [D3D12HelloTriangle.cpp#L578](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/3-DXRTriangle-PerInstanceData/Project/D3D12HelloTriangle.cpp#L578).

# Changing the HitSignature
Now the geometry is set with the triangles, but the shaders have not been adapted yet to use the constant buffer. We first need to modify the root signature of the hit shader to use a constant buffer passed as a root parameter. Contrary to the buffers passed on the heap, root parameters can be passed per instance.

You can see how it's done here: [D3D12HelloTriangle.cpp#L722](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/3-DXRTriangle-PerInstanceData/Project/D3D12HelloTriangle.cpp#L722).

You can see the new buffer in Hit.hlsl here: [Hit.hlsl#L8](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/3-DXRTriangle-PerInstanceData/Project/shaders/Hit.hlsl#L8).

# Adding a Specific Hit Shader for the Plane

## Changing Hit.hlsl
We can add a new Shader inside Hit.hlsl: [Hit.hlsl#L29](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/3-DXRTriangle-PerInstanceData/Project/shaders/Hit.hlsl#L29).

## Changing the RaytracingPipeline
This new shader has to be added to the raytracing pipeline, so:
* First add its symbol to the HitLibrary: [D3D12HelloTriangle.cpp#L766](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/3-DXRTriangle-PerInstanceData/Project/D3D12HelloTriangle.cpp#L766);
* Then add a new hit group: [D3D12HelloTriangle.cpp#L793](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/3-DXRTriangle-PerInstanceData/Project/D3D12HelloTriangle.cpp#L793);
* Then we can add its root signature association: [D3D12HelloTriangle.cpp#L804](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/3-DXRTriangle-PerInstanceData/Project/D3D12HelloTriangle.cpp#L804).

## Changing the ShaderBindingTable
The 4th hit group of the SBT is the one corresponding to the plane. Instead of using `HitGroup`, we now associate it to our newly created hit group, `PlaneHitGroup`.

Since this shader does not require any external data, we can leave its input resources empty: [D3D12HelloTriangle.cpp#L958](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/3-DXRTriangle-PerInstanceData/Project/D3D12HelloTriangle.cpp#L958).

# Further reading
Remember to check [DXR Resources](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials#resources) if you want to know more about the topic or about how to realize these tutorials by yourself.
