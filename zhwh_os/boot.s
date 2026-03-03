.set MULTIBOOT2_HEADER_MAGIC, 0xe85250d6
.set MULTIBOOT_ARCHITECTURE_I386, 0
.set MULTIBOOT_HEADER_TAG_END, 0
.set MULTIBOOT_HEADER_TAG_OPTIONAL, 1
.set MULTIBOOT_HEADER_TAG_FRAMEBUFFER, 5

/* 🔥 定义虚拟地址常量 */
.set VIRT_BASE, 0xC0000000

/* 计算 header 长度和校验和 */
.set MULTIBOOT2_HEADER_LENGTH, multiboot2_header_end - multiboot2_header_start
.set MULTIBOOT2_HEADER_CHECKSUM, -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + MULTIBOOT2_HEADER_LENGTH)

.align 8
.section .multiboot
multiboot2_header_start:
.long MULTIBOOT2_HEADER_MAGIC      /* magic number */
.long MULTIBOOT_ARCHITECTURE_I386   /* architecture */
.long MULTIBOOT2_HEADER_LENGTH      /* header length */
.long MULTIBOOT2_HEADER_CHECKSUM    /* checksum */

/* Framebuffer 请求标签 */
.align 8
.short MULTIBOOT_HEADER_TAG_FRAMEBUFFER   /* type = 5 (framebuffer) */
.short 1       /* flags = REQUIRED */
.long 24                                  /* size */
.long 1024                              /* width = 1024 */
.long 768                                 /* height = 768 */
.long 32           /* bpp (32 bits per pixel, XRGB8888) */
.long 0            /* reserved (unused, must be 0) */

.align 8
.short 0
.short 0
.long 8
multiboot2_header_end:

/* 代码放在 .init 节 */
.section .init
.global loader
loader:

    /* 保存 Multiboot 2 参数到固定位置 */
    movl    %eax, mb_magic
    movl    %ebx, mb_info

    /* AT&T Syntax (GNU as) */
    .set PDE0,   (0 * 4)
    .set PDE300, (0x300 * 4)
    .set PDE301, (0x301 * 4)
    movl    $pd, %eax                    /* MAGIC START! */
    movl $pt  + 3, PDE0(%eax) /*movl    $pt + 3, (%eax)               pd[0] = pt + 3 (WRITE | PRESENT) */
    movl    $pt + 3, PDE300(%eax)         /* pd[0x300] = pt + 3 (映射 0xC0000000-0xC03FFFFF) */
    movl    $pt2 + 3, PDE301(%eax)        /* pd[0x301] = pt2 + 3 (映射 0xC0400000-0xC07FFFFF) */
    /*movl    $pt + 3, 0x302(%eax)          pd[0x302] = pt + 3 (映射 0xC0800000-0xC0BFFFFF) */
    movl    $pt + 3, (0xC00*4)(%eax)         /* pd[0xC00] = pt + 3 (保留兼容性) */

    /* 循环初始化 pt 表（1024项），设置恒等映/*射 */
    /* 映射 0x00000000-0x003FFFFF -> 0x00000000-0x003FFFFF */
    /* 映射 0xC0000000-0xC03FFFFF -> 0x00000000-0x003FFFFF */
    movl    $pt, %edx                    /* edx = pt 基地址 */
    movl    $0, %ecx                     /* ecx = 0（循环计数器） */

.Lloop:
    movl    %ecx, %eax                   /* eax = ecx */
    shll    $12, %eax                    /* eax <<= 12（页框地址） */
    orl     $3, %eax                     /* eax |= 3（WRITE | PRESENT） */
    movl    %eax, (%edx, %ecx, 4)        /* pt[ecx] = eax */

    incl    %ecx                         /* ecx++ */
    cmpl    $1024, %ecx                  /* 1024 项 */
    jne     .Lloop                       /*     goto .Lloop */

    /* 初始化 pt2 表（1024项），映射 0xC0400000-0xC07FFFFF -> 0x00400000-0x007FFFFF */
    /* 🔥 关键修复：正确映射 .bss 段的物理地址 */
    movl    $pt2, %edx                   /* edx = pt2 基地址 */
    movl    $0, %ecx                     /* ecx = 0（循环计数器） */

.Lloop2:
    movl    %ecx, %eax                   /* eax = ecx */
    addl    $0x400, %eax                 /* eax += 0x400（页框号，对应物理地址 0x400000） */
    shll    $12, %eax                    /* eax <<= 12（页框地址） */
    orl     $3, %eax                     /* eax |= 3（WRITE | PRESENT） */
    movl    %eax, (%edx, %ecx, 4)        /* pt2[ecx] = eax */

    incl    %ecx                         /* ecx++ */
    cmpl    $1024, %ecx                  /* 1024 项 */
    jne     .Lloop2                      /*     goto .Lloop2 */

    /* 启用分页 */
    movl    $pd, %eax                /* eax = pd + 3（WRITE | PRESENT） */
    movl    %eax, %cr3                   /* cr3 = eax（加载页目录） */
    movl    %cr0, %eax                    
    orl     $0x80000000, %eax            /* 设置 CR0.PG（启用分页） */
    movl    %eax, %cr0                    

    jmp     higher_half                   /* 跳转到高阶地址空间 */

.Lend:

/* .bss 段（未初始化数据） */
.section .init.bss, "aw", @nobits

pd:    .skip 4096    /* 预留 4KB (0x1000) 给页目录 pd */
pt:    .skip 4096    /* 预留 4KB (0x1000) 给页表 pt */
pt2:    .skip 4096    /* 预留 4KB (0x1000) 给页表 pt2 */

pt3:    .skip 4096    /* 预留 4KB (0x1000) 给页表 pt3 */
pt4:    .skip 4096    /* 预留 4KB (0x1000) 给页表 pt4 */

/* Multiboot 2 参数保存 */
mb_magic: .long 0
mb_info:  .long 0

/* 导出符号供C代码使用 */
.global pd
.global pt
.global pt2
.global pt3
.global pt4
.global mb_magic
.global mb_info
.extern stack_top
.section .text
higher_half:
    # 现在在高阶地址空间运行
    # 设置新堆栈（虚拟地址）
    mov $stack_top, %esp

    xor %ebp,%ebp        # Zero the frame pointer for backtraces.

    # 🔥 修复：使用虚拟地址恢复 Multiboot 2 参数
    # mb_magic 和 mb_info 在 .init.bss 段中，物理地址在 0x00100000 附近
    # 分页后虚拟地址 = 物理地址 + VIRT_BASE
    # 从GDB输出来看，mb_magic的物理地址是0x106000，虚拟地址是0xc0106000
    # 所以我们直接使用虚拟地址从内存中读取
    movl    0xc0106000, %eax
    movl    0xc0106004, %ebx

    # 现在将参数按正确顺序压栈传递给kernel_main
    push %ebx            # 参数2: multiboot2 info地址
    push %eax            # 参数1: magic number

    call kernel_main
    
    # 如果内核主函数返回，进入无限循环
    cli
1:  hlt
    jmp 1b