define show_tf
  set $addr = $esp
  printf "\n==== TRAPFRAME at ESP=%p ====\n", $addr

  printf "General registers:\n"
  printf "EDI=%#x ESI=%#x EBP=%#x OESP=%#x\n", *(unsigned int*)($addr), *(unsigned int*)($addr+4), *(unsigned int*)($addr+8), *(unsigned int*)($addr+12)
  printf "EBX=%#x EDX=%#x ECX=%#x EAX=%#x\n", *(unsigned int*)($addr+16), *(unsigned int*)($addr+20), *(unsigned int*)($addr+24), *(unsigned int*)($addr+28)

  printf "Segment registers:\n"
  printf "GS=%#x FS=%#x ES=%#x DS=%#x\n", *(unsigned int*)($addr+32), *(unsigned int*)($addr+36), *(unsigned int*)($addr+40), *(unsigned int*)($addr+44)

  printf "Trap info:\n"
  printf "TRAPNO=%#x ERR=%#x\n", *(unsigned int*)($addr+48), *(unsigned int*)($addr+52)

  printf "CPU state:\n"
  printf "EIP=%#x CS=%#x EFLAGS=%#x\n", *(unsigned int*)($addr+56), *(unsigned int*)($addr+60), *(unsigned int*)($addr+64)

  printf "User state (if any):\n"
  printf "USER ESP=%#x SS=%#x\n", *(unsigned int*)($addr+68), *(unsigned int*)($addr+72)
end
