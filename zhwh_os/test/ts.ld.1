ENTRY(_start)

SECTIONS
{
    . = 0x08000000;      /* 程序起始地址 */

    /* 主代码段：_start 必须在最前面！ */
    .text : {
        *(.text.start)   /* 先放 _start */
        *(.text .text.*) /* 然后放其他代码 */
    }

    .data : {
        *(.data)
    }

    .bss : {
        __bss_start = .;  /* BSS 起始地址 */
        *(.bss)           /* 所有未初始化数据 */
        __bss_end = .;    /* BSS 结束地址 */
    }

    /* 定义栈的大小和位置 */
    /* 为用户程序分配 128KB 栈空间 */
    .stack ALIGN(4096) : {
        . = . + 128K;
        _stack_top = .;
    }

    /* 确保程序不会超出 32MB 地址空间 */
    /DISCARD/ : {
        *(.note.gnu.build-id)
    }

    /* 定义符号供链接器使用 */
    _end = .;
}

