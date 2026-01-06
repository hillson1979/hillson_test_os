ENTRY(_start)

SECTIONS
{
    . = 0x08000000;      /* 设置程序的起始地址 */
    .text : { *(.text) } /* 程序代码段 */
    .data : { *(.data) } /* 数据段 */
    .bss : { *(.bss) }   /* 未初始化数据段 */
}

