# [Nvidia] DXR "HelloTriangle"  - Important Concepts
This document is a summarize and a rework of the concepts of the Nvidia tutorial, in order to focus attention on some parts and functions in the code.

Again, huge thanks to [Martin-Karl Lefrançois](https://devblogs.nvidia.com/author/mlefrancois/) and [Pascal Gautron](https://devblogs.nvidia.com/author/pgautron/) that made this tutorial, all this work is theirs, i just took it to change and summarize it a bit for educational purposes.

## Contents
- [Introduction](#introduction)
- [Creating or Updating the TopLevelAS](#creating-or-updating-the-toplevelas)
  - [Note](#note)
  - [The function CreateTopLevelAS](#the-function-createtoplevelas)
- [CommandList](#commandlist)
- [OnUpdate](#onupdate)
- [Further reading](#further-reading)

# Introduction
This document require that you understood the basic Raytracing flow made in Tutorial 4, so you can understand the small changes made to make the triangles rotate.

# Creating or Updating the TopLevelAS
## Note
In this tutorial we will consider adding movement in the scene. While this can be done straightforwardly by completely recomputing the acceleration structures, much faster updates can be performed by simply refitting those using the new vertex coordinates or instance transforms. For the sake of simplicity this document only considers updating instance matrices and the top-level acceleration structure, but the same approach can be used to update bottom-level AS as well.
## The function CreateTopLevelAS
First of all add a parameter `bool updateOnly`: [D3D12HelloTriangle.h#L104](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/5-DXRTriangle-AnimatedTriangle/Project/D3D12HelloTriangle.h#L104).

In case only a refit is necessary, there is no need to add the instances in the helper. Similarly, a refit does not change the size of the resulting acceleration structure so the already allocated buffers can be kept.

So we can add a big IF inside the function like this: [D3D12HelloTriangle.cpp#L597](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/5-DXRTriangle-AnimatedTriangle/Project/D3D12HelloTriangle.cpp#L597).

# CommandList
Now we need to call `CreateTopLevelAS` for each frame, to update the mesh data.

You can see the call added here: [D3D12HelloTriangle.cpp#L408](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/5-DXRTriangle-AnimatedTriangle/Project/D3D12HelloTriangle.cpp#L408).

# OnUpdate
This method is called before each render. The time counter will be incremented for each frame, and used to compute a new transform matrix for each triangle.

I also put a different rotation speeds for each triangle, just for fun.

You can see the function here: [D3D12HelloTriangle.cpp#L331](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials/blob/master/5-DXRTriangle-AnimatedTriangle/Project/D3D12HelloTriangle.cpp#L331).

# Further reading
Remember to check [DXR Resources](https://github.com/ScrappyCocco/DirectX-DXR-Tutorials#resources) if you want to know more about the topic or about how to realize these tutorials by yourself.
