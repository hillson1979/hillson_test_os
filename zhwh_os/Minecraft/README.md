# Minecraft

Minecraft is the application layer. It owns the voxel world, chunks, blocks,
player logic, collision, and game rules. Rendering must use OpenGL and the
graph window/context API; it must not access framebuffer memory directly.

The current milestone contains the Block, Chunk, and World data model with a
flat starter area. The next rendering milestone is chunk mesh generation and a
first-person camera using the OpenGL subset in `../opengl`.
