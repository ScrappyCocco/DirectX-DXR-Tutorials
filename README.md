# DirectX 12 DXR Tutorials
Personal repository of tutorials and examples to understand the basics of DirectX 12 Raytracing (DXR).

## Description
This is a personal repository to study and develop various tutorials about DirectX 12 Raytracing. Right now none of the tutorials is made by me, i just take and follow/study a tutorial and upload the result here, cleaning the code from unused functions, writing better includes and comments or stuff like that. I also make sure the project is still buildable and understandable as much as possible. 

**The link to the official tutorial is always in the README of the project directory.**

I will also document some parts of the code using Markdown, so that someone who is new in the topic is able to read and understand the basic concepts just giving a look from the Repository.

The tutorials are in order of increasing difficulty and complexity, and each one require you to understand what has been done in the previous ones.

## Requirements
You need to have some knowledge of Dx12 programming or another low-level graphics api such as Vulkan, in order to understand the basic concepts of shader, pipeline, buffers, and such.

I started this study and repository having some basic knowledge of Vulkan i made developing my small own engine, so i found Dx12 to be quite similar even with its own differences.

The tools you need are:
* Windows 10 with the May 2019 Update (1903);
* Visual Studio 2017 (VS 2019 works updating the building tools to v142 in the project);
* Windows 10 SDK >= May 2019 18362 (10.0.18362.0);
* Dx12 GPU with a compatible DirectX Raytracing driver (Ex: Nvidia driver version 415 or higher).

## Building
Every folder has its own Visual Studio Solution (.sln) that's ready to build and execute if you have all the requirements listed above.

Note that you may need to retarget the Build Tools or the Windows 10 SDK in case of a different version installed on your machine.

If something doesn't work as expected fell free to [open an Issue](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/issues).

## Tutorial list
* [(0) Basic Dx12 Triangle](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/tree/master/0-BasicDxTriangle): Is the basic rendering triangle made in Dx12, useful as starting point;
* [(1) Nvidia - Raytraced Triangle](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/tree/master/1-Dx12DXRTriangle): Raytracing version of the basic Dx12 triangle;
* [(2) Nvidia - Perspective Camera](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/tree/master/2-Dx12DXRTriangle-PerspectiveCamera): The Raytraced version of the triangle seen in Tutorial 1 with the addition of a Perspective Camera;

## Resources
* [Microsoft DirectX 12 Samples](https://github.com/microsoft/DirectX-Graphics-Samples) -> [Raytracing Samples](https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/Samples/Desktop/D3D12Raytracing);
* [Microsoft DirectX Raytracing (DXR) Functional Spec](https://github.com/microsoft/DirectX-Specs/blob/master/d3d/Raytracing.md);
* [Nvidia Raytracing home](https://developer.nvidia.com/rtx/raytracing);
* [Nvidia DirectX Raytracing Basic Tutorial](https://developer.nvidia.com/rtx/raytracing/dxr/DX12-Raytracing-tutorial-Part-1);
* [Nvidia DirectX Raytracing Advanced Tutorial](https://github.com/NVIDIAGameWorks/DxrTutorials).