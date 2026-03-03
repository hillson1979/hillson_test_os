/* 定义内核的入口点 */
ENTRY(loader)

/* 定义内存布局 */
VIRT_BASE = 0xC0000000; /* 3GB - 内核虚拟地址基址 */
PHYS_OFFSET = 0x00100000; /* 1MB - 内核加载的物理地址 */

SECTIONS
{
    /* 内核的加载地址（物理地址）从 1MB 开始 */

    .init 0x00100000:
    {
      /* Multiboot 2 头必须在最前面，前8KB内 */
      KEEP(*(.multiboot))
      PROVIDE(loader = .);
      *(.init)
    }
    
    .init.bss ALIGN(4096) :
    {
        __init_bss_start = .;    /* .init.bss 起始符号 */
        *(.init.bss)
        __init_bss_end = .;      /* .init.bss 结束符号 */
    }
    
    /* 内核的起始位置（物理地址） */
    _kernel_start_physical = .;
    
    /* 设置当前位置到虚拟地址空间 */
    . += ALIGN(VIRT_BASE);
    
    /* 内核的起始位置（虚拟地址） */
    _kernel_start_virtual = .;
    
    PROVIDE(text = .);
    /* 文本段 */
    .text ALIGN(4K) : AT(ADDR(.text) - VIRT_BASE)
    {
        *(.text .text.*)
    }
    
    /* 只读数据段 */
    .rodata ALIGN(4K) : AT(ADDR(.rodata) - VIRT_BASE)
    {
        *(.rodata .rodata.*)
    }
    
    PROVIDE(data = .);
    /* 数据段 */
    .data ALIGN(4K) : AT(ADDR(.data) - VIRT_BASE)
    {
        _data_start = .;
        *(.data .data.*)
        
        _data_end = .;
        _data_end = _data_start + 8192K;
    }
    
    /* BSS 段 */
    .bss ALIGN(4K) : AT(ADDR(.bss) - VIRT_BASE)
    {
        __bss_start = .;    /* BSS 起始符号 */
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;      /* BSS 结束符号 */
    }
    stack_base = ALIGN(__bss_end, 0x1000);
    stack_top = __bss_end +0x2000;       /* 栈顶（高地址） */

    /* 🔥 定义 .bss 段的虚拟地址和物理地址 */
    PROVIDE(bss_virt = ADDR(.bss));
    PROVIDE(bss_phys = ADDR(.bss) - VIRT_BASE);
    
    /* 内核的结束位置（虚拟地址） */
    _kernel_end_virtual = .;
    
    /* 内核的结束位置（物理地址） */
    _kernel_end_physical = . - VIRT_BASE;
    
    /* 丢弃不需要的段 */
    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note.gnu.build-id)
    }
}
