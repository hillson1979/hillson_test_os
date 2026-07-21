ENTRY(_start)

SECTIONS
{
    . = 0xA0000000;      /* 独立地址空间，不与桌面(0x8000000)冲突 */

    .text : {
        *(.text.start)
        *(.text .text.*)
    }

    .data : {
        *(.data)
    }

    .bss : {
        __bss_start = .;
        *(.bss)
        __bss_end = .;
    }

    .stack ALIGN(4096) : {
        . = . + 128K;
        _stack_top = .;
    }

    /DISCARD/ : {
        *(.note.gnu.build-id)
    }

    _end = .;
}
