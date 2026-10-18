# Minecraft

Minecraft is the application layer. It owns the voxel world, chunks, blocks,
player logic, collision, and game rules. Rendering must use OpenGL and the
graph window/context API; it must not access framebuffer memory directly.

The current milestone contains the Block, Chunk, and World data model with a
flat starter area. The next rendering milestone is chunk mesh generation and a
first-person camera using the OpenGL subset in `../opengl`.

## Deployment and launch

The application output is `mc.elf`. It is an external user program and is not
linked into `kernel.bin` or copied into `os.iso`. Put it in the root directory
of a FAT32 USB drive. zhwh_os mounts that filesystem at `/usb`.

Launch it from a desktop application, terminal, or launcher with:

    spawn("/usb/mc.elf", "");

The existing `spawn` syscall opens the file through VFS/FAT32, validates and
loads its ELF segments, creates its user stack and process, and schedules it.
Other applications can use the same deployment model.
