	.file	"simple_syscall_test.c"
	.text
.Ltext0:
	.file 0 "/mnt/f/hillson_test_os/zhwh_os/test" "simple_syscall_test.c"
	.type	syscall_putchar, @function
syscall_putchar:
.LFB0:
	.file 1 "simple_syscall_test.c"
	.loc 1 8 44
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	pushl	%ebx
	subl	$4, %esp
	.cfi_offset 3, -12
	movl	8(%ebp), %eax
	movb	%al, -8(%ebp)
	.loc 1 9 5
	movl	$8, %eax
	movzbl	-8(%ebp), %edx
	movl	%edx, %ebx
#APP
# 9 "simple_syscall_test.c" 1
	int $0x80
# 0 "" 2
	.loc 1 15 1
#NO_APP
	nop
	movl	-4(%ebp), %ebx
	leave
	.cfi_restore 5
	.cfi_restore 3
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE0:
	.size	syscall_putchar, .-syscall_putchar
	.globl	_start
	.type	_start, @function
_start:
.LFB1:
	.loc 1 18 19
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	.loc 1 20 5
	pushl	$84
	call	syscall_putchar
	addl	$4, %esp
	.loc 1 21 5
	pushl	$101
	call	syscall_putchar
	addl	$4, %esp
	.loc 1 22 5
	pushl	$115
	call	syscall_putchar
	addl	$4, %esp
	.loc 1 23 5
	pushl	$116
	call	syscall_putchar
	addl	$4, %esp
	.loc 1 24 5
	pushl	$32
	call	syscall_putchar
	addl	$4, %esp
	.loc 1 25 5
	pushl	$79
	call	syscall_putchar
	addl	$4, %esp
	.loc 1 26 5
	pushl	$75
	call	syscall_putchar
	addl	$4, %esp
	.loc 1 27 5
	pushl	$10
	call	syscall_putchar
	addl	$4, %esp
.L3:
	.loc 1 31 9 discriminator 1
#APP
# 31 "simple_syscall_test.c" 1
	jmp .
# 0 "" 2
#NO_APP
	jmp	.L3
	.cfi_endproc
.LFE1:
	.size	_start, .-_start
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0x60
	.value	0x5
	.byte	0x1
	.byte	0x4
	.long	.Ldebug_abbrev0
	.uleb128 0x1
	.long	.LASF2
	.byte	0x1d
	.long	.LASF0
	.long	.LASF1
	.long	.Ltext0
	.long	.Letext0-.Ltext0
	.long	.Ldebug_line0
	.uleb128 0x2
	.long	.LASF3
	.byte	0x1
	.byte	0x12
	.byte	0x6
	.long	.LFB1
	.long	.LFE1-.LFB1
	.uleb128 0x1
	.byte	0x9c
	.uleb128 0x3
	.long	.LASF4
	.byte	0x1
	.byte	0x8
	.byte	0x14
	.long	.LFB0
	.long	.LFE0-.LFB0
	.uleb128 0x1
	.byte	0x9c
	.long	0x5c
	.uleb128 0x4
	.string	"c"
	.byte	0x1
	.byte	0x8
	.byte	0x29
	.long	0x5c
	.uleb128 0x2
	.byte	0x91
	.sleb128 -16
	.byte	0
	.uleb128 0x5
	.byte	0x1
	.byte	0x6
	.long	.LASF5
	.byte	0
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x1f
	.uleb128 0x1b
	.uleb128 0x1f
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x6
	.uleb128 0x10
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x2e
	.byte	0
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x6
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x7c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x6
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x7a
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x5
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_aranges,"",@progbits
	.long	0x1c
	.value	0x2
	.long	.Ldebug_info0
	.byte	0x4
	.byte	0
	.value	0
	.value	0
	.long	.Ltext0
	.long	.Letext0-.Ltext0
	.long	0
	.long	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF4:
	.string	"syscall_putchar"
.LASF2:
	.string	"GNU C17 13.3.0 -m32 -mtune=generic -march=i686 -g -O0 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fasynchronous-unwind-tables -fstack-clash-protection"
.LASF3:
	.string	"_start"
.LASF5:
	.string	"char"
	.section	.debug_line_str,"MS",@progbits,1
.LASF1:
	.string	"/mnt/f/hillson_test_os/zhwh_os/test"
.LASF0:
	.string	"simple_syscall_test.c"
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
