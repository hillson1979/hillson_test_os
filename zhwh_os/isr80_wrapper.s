/* isr80_wrapper.S   NASM / GAS style (GAS-like shown) */
.global isr80_wrapper
.type isr80_wrapper, @function

isr80_wrapper:
    /* At this point, interrupt_entry has already pushed: ds, es, fs, gs, pusha
       and then pushed vector and called handler (so stack layout is known).
       We need to compute pointer to trapframe on the stack.
       We'll compute TF pointer into EAX, then call syscall_dispatch(EAX).
    */

    pushal                /* save caller-saved regs just in case (optional) */

    /* compute pointer to trapframe on current stack
       NOTE: the offset below is a placeholder. You MUST compute the correct offset
       using gdb and replace OFFSET_TO_TRAPFRAME accordingly.
    */
    movl %esp, %eax
    addl $OFFSET_TO_TRAPFRAME, %eax   /* <-- adjust this constant! */

    pushl %eax            /* push (trapframe_t *) as argument */
    call syscall_dispatch
    addl $4, %esp         /* clean argument */

    popal                 /* restore registers */
    ret

