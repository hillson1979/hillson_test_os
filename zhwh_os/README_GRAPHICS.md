# zhwh_os Graphics Build Boundary

The native kernel graphics stack is built by the existing Makefile:

    make kernel.bin

That target compiles `framebuffer` and the kernel-side `graph` sources with
freestanding flags. The top-level CMake project is user-space only:

    cmake -S . -B build
    cmake --build build

It builds `opengl` and `Minecraft`. User-space code must reach kernel graph
through a future syscall/ABI; it must not link kernel graph object files.

User applications such as `mc.elf` are deployed on a FAT32 USB drive and
launched with `spawn("/usb/mc.elf", "")`. They are not GRUB modules and are not
linked into the kernel.

## User graph ABI

User processes use syscall 87 to query display dimensions and syscall 88 to
present an XRGB8888 back buffer. The ABI never returns the framebuffer physical
address or the kernel mapping at `0xF0000000`. The kernel validates dimensions
and copies each row from user memory before touching the framebuffer.

Public wrappers are declared in `user/graph_user.h`:

    graph_get_display_info(&info);
    graph_present_frame(pixels, width, height, pitch_bytes);

The kernel-side `graph` directory is C-only and freestanding. C++ files that
were part of the earlier desktop prototype were removed; application C++ code
belongs under `Minecraft` or a user-space OpenGL client.
