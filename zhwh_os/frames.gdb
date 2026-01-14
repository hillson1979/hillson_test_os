define show_frames
    printf "\n================ FRAME INSPECTION ================\n"

    # 当前 ESP
    printf "\n[ESP]\n"
    info reg esp

    # 当前 task 和 trapframe
    printf "\n[task / trapframe]\n"
    p current
    p current->tf

    # -----------------------------
    # Frame ① trapframe
    # -----------------------------
    printf "\n[Frame ①: trapframe @ %p]\n", current->tf
    set $tf = current->tf

    printf " pushal regs:\n"
    printf "  EDI = 0x%08x\n", $tf->edi
    printf "  ESI = 0x%08x\n", $tf->esi
    printf "  EBP = 0x%08x\n", $tf->ebp
    printf "  EBX = 0x%08x\n", $tf->ebx
    printf "  EDX = 0x%08x\n", $tf->edx
    printf "  ECX = 0x%08x\n", $tf->ecx
    printf "  EAX = 0x%08x\n", $tf->eax

    printf " trap info:\n"
    printf "  trapno = %d\n", $tf->trapno
    printf "  err    = 0x%08x\n", $tf->err

    printf " iret part:\n"
    printf "  EIP    = 0x%08x\n", $tf->eip
    printf "  CS     = 0x%04x\n", $tf->cs
    printf "  EFLAGS = 0x%08x\n", $tf->eflags
    printf "  ESP    = 0x%08x\n", $tf->esp
    printf "  SS     = 0x%04x\n", $tf->ss

    # -----------------------------
    # Frame ③ iret frame（物理位置）
    # -----------------------------
    printf "\n[Frame ③: raw iret frame in memory]\n"
    printf " (low address → high address)\n"

    set $iret = &$tf->eip
    x/5wx $iret

    # -----------------------------
    # Frame ② switch/context frame
    # -----------------------------
    printf "\n[Frame ②: switch/context frame (below trapframe)]\n"

    # trapframe 下面 5 个 word（通常是 switch_to 保存的）
    set $ctx = (uint32_t*)$tf - 5
    x/5wx $ctx

    printf "\n=============== END FRAME INSPECTION ===============\n"
end