# Reflection Rendering  - Important Concepts
This document is a small list of important concepts, in order to focus attention on some parts and functions in the code. Take the whole example as a reference.

Thanks to [Jorgemagic](https://github.com/Jorgemagic) for his examples.

## Contents
- [Introduction](#introduction)
- [Preprocessor directives](#preprocessor-directives)
- [Main code](#main-code)
- [Shaders](#shaders)
  - [Helpers.hlsli](#helpershlsli)
    - [(Shader Code)](#shader-code)
  - [Shaders.hlsl](#shadershlsl)
    - [(Shader Code)](#shader-code-1)
- [Further reading](#further-reading)
  - [Original tutorial](#original-tutorial)

# Introduction
This document require that you understood the basic Raytracing flow made in [Tutorial 9](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/tree/master/09-Shadow), so you understood the basic of lighting and shadows and can add a basic reflection system.

# Preprocessor directives
This project has:
```cpp
#define _USE_MATH_DEFINES
```
defined in the project file, used mostly to use the constant: `M_PI`.

# Main code
There are no changes in the Cpp code from Tutorial 9, every change is in the shader.

# Shaders
I have not touched the shader files, they are the same Jorgemagic use in [his example](https://github.com/Jorgemagic/CSharpDirectXRaytracing/tree/master/18-Reflection/Data). If you think they can be improved feel free to contribute: [#Contributions](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials#issues--contributions)!

## Helpers.hlsli
### [(Shader Code)](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/10-Reflection/Project/shaders/Helpers.hlsli)

Apart from small changes in the variables at the top of the file, there is a new function: `FresnelReflectanceSchlick`: [Helpers.hlsli#L91](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/10-Reflection/Project/shaders/Helpers.hlsli#L91), to calculate a simplified version of the Fresnel equations about reflected light.

## Shaders.hlsl
### [(Shader Code)](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/10-Reflection/Project/shaders/Shaders.hlsl)

The main difference in the main shader file, is the usage of a reflection ray: [Shaders.hlsl#L121](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/10-Reflection/Project/shaders/Shaders.hlsl#L121) to calculate reflection values, such as the reflected color.

# Further reading

## Original tutorial
You can check [Jorgemagic/CSharpDirectXRaytracing](https://github.com/Jorgemagic/CSharpDirectXRaytracing) if you want to see the original example(s), note that they are all made in C#.
