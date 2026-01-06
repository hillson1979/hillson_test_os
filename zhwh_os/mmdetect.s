section .text
global _start

_start:
    ; 保存 multiboot 信息指针
    mov [multiboot_info], ebx
    mov [multiboot_magic], eax
    
    ; 调用 C 函数处理内存映射
    push ebx
    push eax
    call process_multiboot_info
    add esp, 8
    
    jmp $

section .data
multiboot_magic: dd 0
multiboot_info:  dd 0
