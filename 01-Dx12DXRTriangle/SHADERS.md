# [Nvidia] DXR "HelloTriangle" - Used Shaders

## Contents
- [RayGen shader](#raygen-shader)
    - [(Shader code)](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/01-Dx12DXRTriangle/Project/shaders/RayGen.hlsl)
- [Hit Shader](#hit-shader)
    - [(Shader code)](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/01-Dx12DXRTriangle/Project/shaders/Hit.hlsl)
- [Miss Shader](#miss-shader)
    - [(Shader code)](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/01-Dx12DXRTriangle/Project/shaders/Miss.hlsl)

# RayGen shader 
### [(Shader code)](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/01-Dx12DXRTriangle/Project/shaders/RayGen.hlsl)
The first thing to initialize is the ray payload, that will be used to gather the results of the raytracing. The built-in `DispatchRaysIndex()` provides the 2D coordinates of the current pixel, while `DispatchRaysDimensions()` returns the size of the image being rendered.

Knowing the default camera is located at (0,0,1) and looking in the direction (0,0,−1), we can setup a ray descriptor `RayDesc` representing a ray shooting straight through each pixel by offsetting the x and y coordinates of the ray origin by the normalized floating-point pixel coordinates.

Note that the y coordinate is inverted to match the image indexing convention of DirectX.

The minimum hit distance `TMin` is the equivalent of the camera near clipping plane in the rasterization world, although for raytracing a value of 0 is valid. The value `TMax` is equivalent to the far clipping plane.

Note that unlike rasterization, the ratio between `TMin` and `TMax` does not have any impact on the precision of the intersection calculations.

The ray is then traced using the `TraceRay` call: it will traverse the acceleration structures and invoke the hit group or miss shader depending on the outcome of the traversal.

The ray flags are left to the default `RAY_FLAG_NONE`, but they can be used to optimize the behavior upon hitting a surface, for example to bypass the invocation of any hit shaders (`RAY_FLAG_FORCE_OPAQUE`).

You can find the documentation made by Nvidia of each parameter of `TraceRay` in the Shader code.

The `TraceRay` **is blocking**, in essence similar to a texure lookup, for which the result is available right after calling the texture sampler. Therefore right after `TraceRay` the entire raytracing for that pixel has been done and the payload represents the result of the execution of the various shaders potentially invoked during raytracing.

We can then read the payload and write it to the output buffer.

# Hit Shader 
### [(Shader code)](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/01-Dx12DXRTriangle/Project/shaders/Hit.hlsl)
We replicate the data structure of the vertices in the HLSL code by defining `STriVertex`, which has the same bit mapping as the Vertex structure defined on the CPU side.

In the ClosestHit function of the shader, we can use the built-in `PrimitiveIndex()` call to obtain the index of the triangle we hit, remove the previous hit color computation and replace it by this to access the vertex data.

# Miss Shader 
### [(Shader code)](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/01-Dx12DXRTriangle/Project/shaders/Miss.hlsl)
To slightly differentiate the raster and the raytracing, we will add a simple ramp color background by modifying the Miss function: we simply obtain the coordinates of the currently rendered pixel using and use them to compute a linear gradient.
