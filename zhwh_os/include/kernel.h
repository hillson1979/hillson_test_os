/*
 * 'kernel.h' contains some often-used function prototypes etc
 */
#ifndef KERNEL_H
#define KERNEL_H

void verify_area(void * addr,int count);
volatile void panic(const char * str);
int printf(const char * fmt, ...);
#define printk  printf
void klog_init(void);
void klog_set_dest_str(const char *ip_str, unsigned short port);
int  klog_flush(void);
int tty_write(unsigned ch,char * buf,int count);

#endif
