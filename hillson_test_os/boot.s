/* Multiboot 2 头 - 必须放在 .multiboot 节中 */
.set MULTIBOOT2_HEADER_MAGIC, 0xe85250d6
.set MULTIBOOT_ARCHITECTURE_I386, 0
.set MULTIBOOT_HEADER_TAG_END, 0
.set MULTIBOOT_HEADER_TAG_OPTIONAL, 1

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

/* 结束标签 */
.short MULTIBOOT_HEADER_TAG_END      /* type */
.short 0                             /* flags */
.long 8                              /* size */
multiboot2_header_end:

/* 代码放在 .init 节 */
.section .init
.global loader
loader:

    /* 保存 Multiboot 2 参数到固定位置 */
    movl    %eax, mb_magic
    movl    %ebx, mb_info

    /* AT&T Syntax (GNU as) */
    movl    $pd, %eax                    /* MAGIC START! */
    movl    $pt + 3, (%eax)              /* pd[0] = pt + 3 (WRITE | PRESENT) */
    movl    $pt + 3, 0x300(%eax)         /* pd[0x300] = pt + 3 (映射 0xC0000000-0xC03FFFFF) */
    movl    $pt + 3, 0x301(%eax)         /* pd[0x301] = pt + 3 (映射 0xC0400000-0xC07FFFFF) */
    movl    $pt + 3, 0xC00(%eax)         /* pd[0xC00] = pt + 3 (保留兼容性) */

    /* 循环初始化 pt 表（1024项），设置4MB恒等映射 */
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

    /* 启用分页 */
    movl    $pd + 3, %eax                /* eax = pd + 3（WRITE | PRESENT） */
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

.section .text
higher_half:
    # 现在在高阶地址空间运行
    # 设置新堆栈（虚拟地址）
    mov $stack_top, %esp

    xor %ebp,%ebp        # Zero the frame pointer for backtraces.

    # 从固定位置恢复 Multiboot 2 参数
    movl    mb_magic, %eax
    movl    mb_info, %ebx

    # 现在将参数按正确顺序压栈传递给kernel_main
    push %ebx            # 参数2: multiboot2 info地址
    push %eax            # 参数1: magic number

    call kernel_main
    
    # 如果内核主函数返回，进入无限循环
    cli
1:  hlt
    jmp 1b

.section .bss
.align 8192       /* 按 8KB 对齐 */

.global stack_base /* 声明全局符号 */
.global stack_top
stack_base:
    .skip 0x2000  /* 预留 8192 字节 */
stack_top:            /* 标签 stack 指向末尾 */
