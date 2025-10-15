extern int sys_printf(const char *fmt);
extern void sys_exit(int code);

void _start() {
    sys_printf("Hello from user program!\n");
    //sys_exit(0);
    for(;;);  // 永不返回
}

