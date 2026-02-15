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
    *(.init.bss)
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
        *(COMMON)
        *(.bss .bss.*)
        . = ALIGN(8K);       /* 栈对齐（可选） */
        stack_base = .;    /* 栈底（低地址） */
        . += 8K;             /* 栈大小 16KB */
        stack_top = .;       /* 栈顶（高地址） */
    }
    
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
