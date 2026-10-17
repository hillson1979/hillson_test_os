# opengl

This directory contains the first, small OpenGL-compatible API for zhwh_os.
The implementation strategy is software-first: implement the subset needed by
the Minecraft-style voxel engine, rasterize into graph's framebuffer, and add a
hardware backend later.

Current API skeleton:

- `glClearColor`
- `glClear`
- `glEnable`
- `glViewport`
- `glDrawArrays`
- `glSetContext`

Planned order:

1. Context and OpenGL state machine.
2. Vertex and index buffers.
3. Textures and texture sampling.
4. Shader-compatible fixed pipeline or a small GLSL subset.
5. Depth testing and triangle rasterization.
6. Optional TinyGL or GPU backend integration.

The application dependency remains `Minecraft -> OpenGL`. Access to the
kernel-side `graph` layer will be added through a syscall/ABI; the user-space
targets must not link kernel graph objects directly.
