.set MAGIC, 0x1BADB002
.set FLAGS, 1<<1
.set CHECKSUM, -(MAGIC + FLAGS)

.align 4
.section .init
multiboot:
.long MAGIC
.long FLAGS
.long CHECKSUM

.global loader
loader:

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

/* 导出符号供C代码使用 */
.global pd
.global pt
.global pt2
.global pt3
.global pt4

.section .text
higher_half:
    # 现在在高阶地址空间运行
    # 设置新堆栈（虚拟地址）
    mov $stack_top, %esp
    
    xor %ebp,%ebp        # Zero the frame pointer for backtraces.
    
    push %ebx 
    # 调用内核主函数
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
