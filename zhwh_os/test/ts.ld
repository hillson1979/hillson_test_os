/* ts.ld - 测试程序链接脚本 */

OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)

SECTIONS
{
    . = 0x08000000;

    .text : {
        *(.text)
    }

    /* 确保入口点对齐 */
    . = ALIGN(4);

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}
