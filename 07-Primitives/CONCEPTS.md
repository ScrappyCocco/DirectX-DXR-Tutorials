# Primitives Rendering  - Important Concepts
This document is a small list of important concepts, in order to focus attention on some parts and functions in the code. Take the whole example as a reference.

Thanks to [Jorgemagic](https://github.com/Jorgemagic) for his examples.

## Contents
- [Introduction](#introduction)
- [Preprocessor directives](#preprocessor-directives)
- [Primitive(s) creation](#primitives-creation)
  - [Vertex/Index creation](#vertexindex-creation)
  - [Buffer creation and fill](#buffer-creation-and-fill)
- [Acceleration Structure creation](#acceleration-structure-creation)
  - [Possible errors](#possible-errors)
- [Further reading](#further-reading)
  - [Original tutorial](#original-tutorial)

# Introduction
This document require that you understood the basic Raytracing flow made in [Tutorial 6](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/tree/master/06-DXRTriangle-Rework), so know the basics to render multiple Raytraced triangles, and can go in-depth to see how to render a more complex mesh.

The basics are exactly the same of the past 6 tutorials, but is very important you understood the project structure of tutorial 6, that doesn't use any Nvidia helper class.

# Preprocessor directives
This project has:
```cpp
#define _USE_MATH_DEFINES
```
defined in the project file, used mostly to use the constant: `M_PI`.

# Primitive(s) creation
One important change in this tutorial is that we will use actual meshes made of vertex/index buffers than a simple triangle.

## Vertex/Index creation
So, from the basic example, we now have this class: [Primitives.h](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/07-Primitives/Project/Source/Utils/Primitives.h) that only define Sphere and Cube (to keep it simple), but every mesh can be defined, for example a Capsule as in the original example.

## Buffer creation and fill
That call is made from here: [AccelerationStructures.cpp#L59](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/07-Primitives/Project/Source/Utils/AccelerationStructures.cpp#L59) that also create the actual DX buffers, and copy the data into them.

# Acceleration Structure creation
The creation of the AS is not very different from tutorial 6. After calling the method that create the Primitive buffers, the creation is very similar to the past tutorial: [AccelerationStructures.cpp#L110](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/07-Primitives/Project/Source/Utils/AccelerationStructures.cpp#L110). Just for convenience i let a `for` cycle for a single element.

Also note that this tutorial doesn't update any position/rotation, so you will not find any update call (for example for updating the AS).

## Possible errors

### Define transform
Always remember to define important fields, such as Transform here: [AccelerationStructures.cpp#L220](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/07-Primitives/Project/Source/Utils/AccelerationStructures.cpp#L220). Leaving it undefined will lead to errors that will not render the mesh, as the position is probably broken.

### Shader import
If any of your shaders use the `#include` directive, remember to create and use the **include handler** when you compile the shader, look at: [RTPipeline.cpp#L54](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/07-Primitives/Project/Source/Utils/RTPipeline.cpp#L54). Without it, the code will not work.

# Further reading

## Original tutorial
You can check [Jorgemagic/CSharpDirectXRaytracing](https://github.com/Jorgemagic/CSharpDirectXRaytracing) if you want to see the original example(s), note that they are all made in C#.
