# DirectX 12 DXR Tutorials
Personal repository of tutorials and examples to understand the basics of DirectX 12 instructions and DirectX 12 Raytracing (DXR).

## Description
This is a personal repository to study and develop various tutorials about DirectX 12 Raytracing. Right now none of the tutorials is made by me, I just take and follow/study a tutorial or an example and upload the result here, cleaning the code from unused functions, writing better includes and comments or stuff like that. I also make sure the project is still buildable and understandable as much as possible.

I made this also because, from a beginner point of view, lots of tutorials are full of unused variables, compilation errors, and so on, and I wanted to provide clean resources anyone could consider to read/use.

**The link to the original tutorial/example is always in the README of the project directory.**

I will also document some parts of the code using Markdown, so that someone who is new in the topic is able to read and understand the basic concepts just giving a look from the Repository.

The tutorials are in order of increasing difficulty and complexity, and each one require you to understand what has been done in the previous ones. Also because every tutorial starts with the code of the previous one, adding or changing some functions.

More tutorials might be added in the future.

## Tutorial list

* [(0) Microsoft - Basic Dx12 Triangle](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/tree/master/0-BasicDxTriangle): Is the basic rendering triangle made in Dx12, useful as starting point (No Raytracing);
* [(1) Nvidia - Raytraced Triangle](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/tree/master/1-Dx12DXRTriangle): Raytracing version of the basic Dx12 triangle;
* [(2) Nvidia - Perspective Camera](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/tree/master/2-Dx12DXRTriangle-PerspectiveCamera): The Raytraced version of the triangle seen in Tutorial 1 with the addition of a Perspective Camera;
* [(3) Nvidia - Per Instance Data](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/tree/master/3-DXRTriangle-PerInstanceData): The Raytraced triangle, now with the addition of more geometry, using "PerInstanceCommandBuffers" and "PerGeometryHitShader";
* [(4) Nvidia - Another Ray Type](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/tree/master/4-DXRTriangle-AnotherRayType): This tutorial add another ray to the scene, used for shadows;
* [(5) Nvidia - Animated Scenes](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/tree/master/5-DXRTriangle-AnimatedTriangle): This is the Raytracing triangles of Tutorial 4, with few changes to make the triangles rotate, to show what to change for a moving object;
* [(6) Nvidia - Refitting](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/tree/master/6-DXRTriangle-Rework): This is the first tutorial **without** Nvidia helper classes. So you can see the creation and the usage of every resource;
* [(7) Primitives](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/tree/master/7-Primitives): This tutorial shows how to create a Raytracing Acceleration Structure from vertex and index geometry buffers, rendering a Sphere or a Cube.
* [Future...?]

## Requirements

### Intro
You need to have some knowledge of Dx12 programming or another low-level graphics api such as Vulkan, in order to understand the basic concepts of shader, pipeline, buffers, and such.

I started this study and repository having some basic knowledge of Vulkan i made developing my small own engine, so i found Dx12 to be quite similar even with its own differences.

### Externals
From tutorial 6, also [glm](https://github.com/g-truc/glm) is necessary, so make sure that the folder `_externals/glm` is not empty.

If is empty, initialize the submodule with:
```
git submodule update --init --recursive
```

### Tools
The tools you need are:

* Windows 10 with the May 2019 Update (1903);
* Visual Studio 2017 (VS2019 should works updating the building tools to v142 in the project);
* Windows 10 SDK >= May 2019 18362 (10.0.18362.0);
* Dx12 GPU with a compatible DirectX Raytracing driver (Ex: Nvidia driver version 415 or higher).

## Building
Every folder has its own Visual Studio Solution (.sln) that's ready to build and execute if you have all the requirements listed above.

Note that you may need to retarget the Build Tools or the Windows 10 SDK in case of a different version installed on your machine.

If something doesn't work as expected, or you wish to change anything, feel free to [open an Issue](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/issues) so we can discuss about it.

## Resources

* [Microsoft DirectX 12 Samples](https://github.com/microsoft/DirectX-Graphics-Samples) -> [Raytracing Samples](https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/Samples/Desktop/D3D12Raytracing);
* [Microsoft DirectX Raytracing (DXR) Functional Spec](https://github.com/microsoft/DirectX-Specs/blob/master/d3d/Raytracing.md);
* [Nvidia Raytracing home](https://developer.nvidia.com/rtx/raytracing);
* [Nvidia DirectX Raytracing Basic Tutorial](https://developer.nvidia.com/rtx/raytracing/dxr/DX12-Raytracing-tutorial-Part-1);
* [Nvidia DirectX Raytracing Advanced Tutorial](https://github.com/NVIDIAGameWorks/DxrTutorials);
* [Jorgemagic C# DirectX Raytracing Examples](https://github.com/Jorgemagic/CSharpDirectXRaytracing) (even if not made in C++ these examples are very useful!).
