# zhwh_os Graphics Build Boundary

The native kernel graphics stack is built by the existing Makefile:

    make kernel.bin

That target compiles `framebuffer` and the kernel-side `graph` sources with
freestanding flags. The top-level CMake project is user-space only:

    cmake -S . -B build
    cmake --build build

It builds `opengl` and `Minecraft`. User-space code must reach kernel graph
through a future syscall/ABI; it must not link kernel graph object files.

The kernel-side `graph` directory is C-only and freestanding. C++ files that
were part of the earlier desktop prototype were removed; application C++ code
belongs under `Minecraft` or a user-space OpenGL client.
