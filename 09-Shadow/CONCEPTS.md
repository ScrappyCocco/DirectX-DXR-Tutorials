# Shadow Rendering  - Important Concepts
This document is a small list of important concepts, in order to focus attention on some parts and functions in the code. Take the whole example as a reference.

Thanks to [Jorgemagic](https://github.com/Jorgemagic) for his examples.

## Contents
- [Introduction](#introduction)
- [Preprocessor directives](#preprocessor-directives)
- [Acceleration Structures / Primitives](#acceleration-structures--primitives)
- [RT Pipeline](#rt-pipeline)
- [Main file](#main-file)
- [Shaders](#shaders)
  - [Helpers.hlsli](#helpershlsli)
    - [(Shader Code)](#shader-code)
  - [Shaders.hlsl](#shadershlsl)
    - [(Shader Code)](#shader-code-1)
- [Further reading](#further-reading)
  - [Original tutorial](#original-tutorial)

# Introduction
This document require that you understood the basic Raytracing flow made in [Tutorial 8](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/tree/master/08-Lighting), so you understood the basic of lighting and can add a basic shadow.

# Preprocessor directives
This project has:
```cpp
#define _USE_MATH_DEFINES
```
defined in the project file, used mostly to use the constant: `M_PI`.

# Acceleration Structures / Primitives
Some changes have been made to allow an easier creation of primitives, to allow for example to create both the sphere and the plane and their acceleration structures more easily.

So there is a new plane primitive creation: [Primitives.cpp#L264](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/09-Shadow/Project/Source/Utils/Primitives.cpp#L264).

And you can see that `createPrimitive` is now more general: [AccelerationStructures.cpp#L65](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/09-Shadow/Project/Source/Utils/AccelerationStructures.cpp#L65), `createBottomLevelAS` too that now read a generic shape as parameter: [AccelerationStructures.cpp#L118](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/09-Shadow/Project/Source/Utils/AccelerationStructures.cpp#L118).

So now we also have `createPlaneBottomLevelAS`:[AccelerationStructures.cpp#L184](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/09-Shadow/Project/Source/Utils/AccelerationStructures.cpp#L184) and `createPrimitiveBottomLevelAS`:[AccelerationStructures.cpp#L197](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/09-Shadow/Project/Source/Utils/AccelerationStructures.cpp#L197).

# RT Pipeline

Apart from the new shader entry point of course, there is a new descriptor entry in the Hit descriptor: [RTPipeline.cpp#L123](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/09-Shadow/Project/Source/Utils/RTPipeline.cpp#L123).

Also it now use
```cpp
desc.range.size()
```
To be more general instead of a static number that must be changed everytime.

# Main file
Except some small changes in the Acceleration Structures usage/creation, again the main change is in the RT pipeline state/shader table.

# Shaders
I have not touched the shader files, they are the same Jorgemagic use in [his example](https://github.com/Jorgemagic/CSharpDirectXRaytracing/tree/master/17-Shadow/Data). If you think they can be improved feel free to contribute: [#Contributions](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials#issues--contributions)!

## Helpers.hlsli
### [(Shader Code)](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/09-Shadow/Project/shaders/Helpers.hlsli)

Added a couple of variables and small changes to consider if an object is in shadow and/or the shadowFactor in diffuse.

## Shaders.hlsl
### [(Shader Code)](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/09-Shadow/Project/shaders/Shaders.hlsl)

Apart from some changes such as the ShadowPayload, the main difference is the shadow ray: [Shaders.hlsl#L96](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/09-Shadow/Project/shaders/Shaders.hlsl#L96).

# Further reading

## Original tutorial
You can check [Jorgemagic/CSharpDirectXRaytracing](https://github.com/Jorgemagic/CSharpDirectXRaytracing) if you want to see the original example(s), note that they are all made in C#.
