/**
 * readlog.elf — 读取并显示 /kern.log
 *
 * 用法: GRUB 启动项选择 "Read Kernel Log"
 * 显示上次启动的内核日志（仅热重启有效）
 */
void _start() {
    int fd = open("/kern.log", 0);
    if (fd < 0) {
        write(1, "ERROR: Cannot open /kern.log\n", 30);
        exit(1);
    }

    char buf[256];
    int n;
    while ((n = read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = 0;
        write(1, buf, n);
    }
    close(fd);
    exit(0);
}
