# graph

Native graphics foundation for zhwh_os. The dependency direction is:

    OpenGL -> graph -> framebuffer

`graph` owns windows, surfaces, contexts, event polling, buffer swapping, and
the connection to the platform framebuffer. It does not expose Minecraft or
game-specific drawing functions.

The current backend is a framebuffer hook. `framebuffer_backend.c` is a
platform stub that must be replaced by the zhwh_os display driver.

Public headers:

- `framebuffer/framebuffer.h`: display memory and present contract.
- `graph/context.h`: rendering context and viewport.
- `graph/window.h`: window lifecycle and swap/poll operations.
