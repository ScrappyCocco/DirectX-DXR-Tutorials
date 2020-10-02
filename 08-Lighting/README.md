![Screenshot of Dx12 DXR Lighting rendering](ReadmeMedia/screenshot.png)

# Lighting Rendering
This example shows how to lighting a mesh using Raytracing pipeline. The acceleration Structures only have information about the vertex position of the mesh, so we need to pass vertexBuffer and indexBuffer information to the shader to reconstruct the vertex information after a hit.

This is an example i found from Jorge Cantón, that you can find here: [Jorgemagic/CSharpDirectXRaytracing](https://github.com/Jorgemagic/CSharpDirectXRaytracing). The project structure is the same too, because i found it quite clean and easy to read.

## Points of interest
See the [Important Concepts](CONCEPTS.md) document, that you can find in this folder.
