# 先加载内核符号
file kernel.bin

# 附加到 QEMU
target remote :1234

# 在 iret 处停下
break *interrupt_exit

# 运行到断点
c

# 单步执行 iret（会跳转到用户态入口）
si

# 在用户态入口设置硬件断点
hbreak *0x08000000

# 继续运行，直到用户代码入口
c

# 打印关键信息
echo "\n--- user entry ---\n"
info registers
x/16i $eip

