# GDB 脚本：检查 task->tf 的偏移量
# 使用方法：在 GDB 中运行：source check_task_tf.gdb

printf "=== Checking task->tf offset ===\n"
printf "ECX (task pointer) = 0x%x\n", $ecx
printf "\n"
printf "Checking different offsets:\n"
printf "  offset 152: *(uint32_t*) = 0x%x\n", *(unsigned long*)($ecx + 152)
printf "  offset 156: *(uint32_t*) = 0x%x\n", *(unsigned long*)($ecx + 156)
printf "  offset 160: *(uint32_t*) = 0x%x\n", *(unsigned long*)($ecx + 160)
printf "\n"
printf "The correct task->tf value should be 0xC02A3114\n"
printf "TASK_IFRAME constant = 156\n"
printf "\n"
printf "If offset 156 shows 0xC02A3114, then TASK_IFRAME is correct.\n"
printf "If offset 152 shows 0xC02A3114, then TASK_IFRAME should be 152.\n"
continue
