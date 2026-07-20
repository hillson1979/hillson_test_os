ENTRY(_start)

SECTIONS
{
    . = 0x08000000;      /* User program load address */

    /* Code — _start must be first */
    .text : {
        *(.text.start)   /* _start function (in cxx_runtime.cpp) */
        *(.text .text.*) /* All other code */
    }

    /* Read-only data */
    .rodata : {
        *(.rodata .rodata.*)
    }

    /* ============================================================
       C++ specific sections
       ============================================================ */

    /* Global constructors (run before main) */
    .init_array : {
        __init_array_start = .;
        KEEP(*(.init_array .init_array.*))
        __init_array_end = .;
    }

    /* Global destructors (run at exit — we stub __cxa_atexit) */
    .fini_array : {
        __fini_array_start = .;
        KEEP(*(.fini_array .fini_array.*))
        __fini_array_end = .;
    }

    /* Initialized data */
    .data : {
        *(.data)
    }

    /* Uninitialized data */
    .bss : {
        __bss_start = .;
        *(.bss)
        *(COMMON)
        __bss_end = .;
    }

    /* Stack — 128 KB */
    .stack ALIGN(4096) : {
        . = . + 128K;
        _stack_top = .;
    }

    /* Discard sections we don't need */
    /DISCARD/ : {
        *(.note.gnu.build-id)
        *(.eh_frame)           /* Exception handling frames */
        *(.eh_frame_hdr)       /* EH frame header */
        *(.gcc_except_table)   /* GCC exception tables */
        *(.comment)
    }

    _end = .;
}
