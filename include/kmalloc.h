#ifndef KMALLOC_H
#define KMALLOC_H

// 通用内核内存分配
void *kmalloc(unsigned int size);
void kfree(void *ptr);

// 分配并清零内存
void *kzalloc(unsigned int size);

// 打印分配统计
void kmalloc_print_stats(void);

// 早期内存分配（用于内核初始化阶段）
void *kmalloc_early(unsigned int size);

#endif
