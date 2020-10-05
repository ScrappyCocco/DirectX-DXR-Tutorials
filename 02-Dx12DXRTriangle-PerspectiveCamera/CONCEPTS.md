# [Nvidia] DXR "HelloTriangle"  - Important Concepts
This document is a summarize and a rework of the concepts of the Nvidia tutorial, in order to focus attention on some parts and functions in the code.

Again, huge thanks to [Martin-Karl Lefrançois](https://devblogs.nvidia.com/author/mlefrancois/) and [Pascal Gautron](https://devblogs.nvidia.com/author/pgautron/) that made this tutorial, all this work is theirs, i just took it to change and summarize it a bit for educational purposes.

## Contents
- [Introduction](#introduction)
- [Creating the Camera](#creating-the-camera)
  - [Creating the Camera](#creating-the-camera-1)
  - [Updating the Camera](#updating-the-camera)
- [Updating Resources](#updating-resources)
  - [ShaderResourceHeap](#shaderresourceheap)
  - [CreateRayGenSignature](#createraygensignature)
- [LoadAssets](#loadassets)
- [CommandList](#commandlist)
- [Shaders](#shaders)
  - [shaders.hlsl](#shadershlsl)
  - [RayGen.hlsl](#raygenhlsl)
- [Further reading](#further-reading)

# Introduction
This document require that you understood the basic Raytracing flow made in [Tutorial 1](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/tree/master/01-Dx12DXRTriangle), once you know the basics you can start to read how to edit the standard camera to a perspective camera.

# Changes from tutorial
In this code there is a small change from the Nvidia tutorial, to fix the shader table alignment error. To make it simple it just force the size of each record to be a multiple of 64 instead of 32.

To see what changed in the code see commit: [5ddcb75](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/commit/5ddcb75460742f5675383a4299815844f26ce0c3).

# Creating the Camera
## Creating the Camera
The new Camera is created and allocated here: [D3D12HelloTriangle.cpp#L974](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/02-Dx12DXRTriangle-PerspectiveCamera/Project/D3D12HelloTriangle.cpp#L974). This method is creating a buffer to contain all matrices. We then create a heap referencing the camera buffer, that will be used in the rasterization path.

## Updating the Camera
The Camera also need to be updated, and that is done here: [D3D12HelloTriangle.cpp#L1004](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/02-Dx12DXRTriangle-PerspectiveCamera/Project/D3D12HelloTriangle.cpp#L1004). As you can see, the following function creates and copies the viewmodel and perspective matrices of the camera.

That function is called on Update() here: [D3D12HelloTriangle.cpp#L324](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/02-Dx12DXRTriangle-PerspectiveCamera/Project/D3D12HelloTriangle.cpp#L324).

# Updating Resources

## ShaderResourceHeap
The camera buffer needs to be accessed by the raytracing path as well. To this end, we modify `CreateShaderResourceHeap` and add a reference to the camera buffer in the heap used by the raytracing. The heap then needs to be made bigger, to contain the additional reference. You can see it here: [D3D12HelloTriangle.cpp#L866](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/02-Dx12DXRTriangle-PerspectiveCamera/Project/D3D12HelloTriangle.cpp#L866).

At the end of the method, we add the actual camera buffer reference, as you can see here: [D3D12HelloTriangle.cpp#L898](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/02-Dx12DXRTriangle-PerspectiveCamera/Project/D3D12HelloTriangle.cpp#L898).

## CreateRayGenSignature
Since we have changed our heap and want to access the new matrices, the Root Signature of the RayGen shader must be changed.

So we add the extra entry to access the constant buffer through the `b0` register, like this:
[D3D12HelloTriangle.cpp#L707](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/02-Dx12DXRTriangle-PerspectiveCamera/Project/D3D12HelloTriangle.cpp#L707).

# LoadAssets
The buffer starts with 2 matrices:

* The view matrix, representing the location of the camera;
* The projection matrix, a simple representation of the behavior of the camera lens.

Those matrices are the classical ones used in the rasterization process, projecting the world-space positions of the vertices into a unit cube.

**However**, to obtain a raytracing result consistent with the rasterization, we need to do the opposite: the rays are initialized as if we had an orthographic camera located at the origin. We then need to transform the ray origin and direction into world space, using the inverse view and projection matrices. The camera buffer stores all 4 matrices, where the raster and raytracing paths will access only the ones needed.

We now need to indicate that the rasterization shaders will use the camera buffer, by modifying their root signature. The shader now takes one constant buffer (CBV) parameter, accessible from the currently bound heap: [D3D12HelloTriangle.cpp#L185](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/02-Dx12DXRTriangle-PerspectiveCamera/Project/D3D12HelloTriangle.cpp#L185).

# CommandList
Until now the rasterization path did not require access to any resources, hence we did not bind any heap for use by the shaders. Now we need to add the Camera: [D3D12HelloTriangle.cpp#L387](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/02-Dx12DXRTriangle-PerspectiveCamera/Project/D3D12HelloTriangle.cpp#L387).

# Shaders
## shaders.hlsl
The last step to use the camera buffer for rasterization is to use the newly created buffer inside the shader. Since the buffer is associated to the register `b0`, we add the declaration at the beginning of the file. Note that since only the view and projection matrices are required, and they are at the beginning of the buffer, we only declare those in the shader: [shaders.hlsl#L13](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/02-Dx12DXRTriangle-PerspectiveCamera/Project/shaders/shaders.hlsl#L13).

And then we modify the vertex shader to use the matrices: [shaders.hlsl#L24](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/02-Dx12DXRTriangle-PerspectiveCamera/Project/shaders/shaders.hlsl#L24).

## RayGen.hlsl
The raytracing mode requires changes in the ray generation shader. For this we first add the declaration of the camera buffer. Here, we use all the available matrices: [RayGen.hlsl#L10](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/02-Dx12DXRTriangle-PerspectiveCamera/Project/shaders/RayGen.hlsl#L10).

The ray generation then uses the inverse matrices to generate a ray: using a ray starting on a [0,1]x[0,1] square on the XY plane, and with a direction along the Z axis, we apply the inverse transforms to generate a perspective projection at the actual camera location.

For this, we replace the origin and direction in the `RayDesc` initialization: [RayGen.hlsl#L28](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/02-Dx12DXRTriangle-PerspectiveCamera/Project/shaders/RayGen.hlsl#L28).

# Further reading
Remember to check [DXR Resources](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials#resources) if you want to know more about the topic or about how to realize these tutorials by yourself.
