# Lighting Rendering  - Important Concepts
This document is a small list of important concepts, in order to focus attention on some parts and functions in the code. Take the whole example as a reference.

Thanks to [Jorgemagic](https://github.com/Jorgemagic) for his examples.

## Contents
- [Introduction](#introduction)
- [Preprocessor directives](#preprocessor-directives)
- [Accelleration Structures / RT Pipeline](#accelleration-structures--rt-pipeline)
- [Main file](#main-file)
- [Shaders](#shaders)
  - [Helpers.hlsli](#helpershlsli)
    - [(Shader Code)](#shader-code)
  - [Shaders.hlsl](#shadershlsl)
    - [(Shader Code)](#shader-code-1)
- [Further reading](#further-reading)
  - [Original tutorial](#original-tutorial)

# Introduction
This document require that you understood the basic Raytracing flow made in [Tutorial 7](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/tree/master/07-Primitives), so you know how the primitive rendering is done, and can add a basic lighting to it.

The project structure is the same from Tutorial 7.

# Preprocessor directives
This project has:
```cpp
#define _USE_MATH_DEFINES
```
defined in the project file, used mostly to use the constant: `M_PI`.

# Accelleration Structures / RT Pipeline
One small difference in the Accelleration Structures files, is that the created primitive is now a static pointer: [AccelerationStructures.h#L34](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/08-Lighting/Project/Source/Utils/AccelerationStructures.h#L34), this to access its index/vertex buffer directly: [D3D12Lighting.cpp#L457](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/08-Lighting/Project/Source/D3D12Lighting.cpp#L457).

The RTPipeline has now a specific Hit descriptor: [RTPipeline.cpp#L117](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/08-Lighting/Project/Source/Utils/RTPipeline.cpp#L117).

# Main file
The most important and noticeable difference in the main file, is the different `createRtPipelineState`: [D3D12Lighting.cpp#L269](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/08-Lighting/Project/Source/D3D12Lighting.cpp#L269) and `createShaderTable`: [D3D12Lighting.cpp#L353](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/08-Lighting/Project/Source/D3D12Lighting.cpp#L353).

# Shaders
I have not touched the shader files, they are the same Jorgemagic use in [his example](https://github.com/Jorgemagic/CSharpDirectXRaytracing/tree/master/16-Lighting/Data). If you think they can be improved feel free to contribute: [#Contributions](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials#issues--contributions)!

## Helpers.hlsli
### [(Shader Code)](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/08-Lighting/Project/shaders/Helpers.hlsli)

A lot of new variables and function have been added to the Helper file, to calculate the whole Lighting.

## Shaders.hlsl
### [(Shader Code)](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/08-Lighting/Project/shaders/Shaders.hlsl)

You can have a look at the new `HitAttribute` function used: [Shaders.hlsl#L58](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/08-Lighting/Project/shaders/Shaders.hlsl#L58), and how to closesthit (`chs`) changed: [Shaders.hlsl#L66](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/08-Lighting/Project/shaders/Shaders.hlsl#L66).

# Further reading

## Original tutorial
You can check [Jorgemagic/CSharpDirectXRaytracing](https://github.com/Jorgemagic/CSharpDirectXRaytracing) if you want to see the original example(s), note that they are all made in C#.
