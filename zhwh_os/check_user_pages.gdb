set $pd_base = 0x101000
set $elf_start_pde_idx = 32
set $elf_num_pages = 4
set $stack_start_pde_idx = 767
set $stack_num_pages = 4

define print_elf_pages
    printf "===== ELF 前 %d 页映射 =====\n", $elf_num_pages
    set $i = 0
    while $i < $elf_num_pages
        set $pde_addr = $pd_base + ($elf_start_pde_idx + $i)*4
        set $pde_val = *(unsigned int *)$pde_addr
        printf "ELF PDE[%d]: 0x%x\n", $elf_start_pde_idx + $i, $pde_val
        set $i = $i + 1
    end
end

define print_stack_pages
    printf "===== 用户栈页映射 =====\n"
    set $i = 0
    while $i < $stack_num_pages
        set $stack_va = 0xBFFFF000 - $i*0x1000
        printf "Stack page %d: va=0x%x\n", $i, $stack_va
        set $i = $i + 1
    end
end

print_elf_pages
print_stack_pages
