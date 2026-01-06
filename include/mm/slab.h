#ifndef SLAB_H
#define SLAB_H

#include "types.h"
#include "spinlock.h"

// Slab 缓存标志
#define SLAB_CACHE_DMA    (1 << 0)  // 使用 DMA 内存
#define SLAB_CACHE_PANIC  (1 << 1)  // 分配失败时 panic
#define SLAB_CACHE_NOFS   (1 << 2)  // 分配过程中不使用文件系统

// Slab 对象状态
#define SLAB_OBJECT_FREE  0
#define SLAB_OBJECT_USED  1

// Slab 结构
typedef struct slab {
    struct slab* next;        // 下一个 slab
    struct slab* prev;        // 上一个 slab
    uint32_t* free_objects;   // 空闲对象位图
    uint32_t free_count;      // 空闲对象数量
    uint32_t total_count;     // 总对象数量
    uint32_t flags;           // slab 标志
    char* start;              // 对象开始地址
} slab_t;

// Slab 缓存结构
typedef struct slab_cache {
    const char* name;         // 缓存名称
    uint32_t object_size;     // 对象大小
    uint32_t align;           // 对象对齐要求
    uint32_t flags;           // 缓存标志
    uint32_t num_per_slab;    // 每个 slab 中的对象数量
    
    slab_t* full_slabs;       // 已满的 slab 链表
    slab_t* partial_slabs;    // 部分满的 slab 链表
    slab_t* empty_slabs;      // 空的 slab 链表
    
    spinlock_t lock;          // 保护缓存的锁
    
    // 构造和析构函数
    void (*ctor)(void* obj);
    void (*dtor)(void* obj);
} slab_cache_t;

// 初始化 slab 分配器
int slab_init(void);

// 创建 slab 缓存
slab_cache_t* slab_cache_create(const char* name, uint32_t size, uint32_t align, uint32_t flags, 
                               void (*ctor)(void*), void (*dtor)(void*));

// 销毁 slab 缓存
int slab_cache_destroy(slab_cache_t* cache);

// 从缓存中分配对象
void* slab_alloc(slab_cache_t* cache);

// 释放对象到缓存
void slab_free(slab_cache_t* cache, void* obj);

// 重新调整缓存大小
int slab_cache_resize(slab_cache_t* cache, uint32_t new_size);

// 获取缓存统计信息
void slab_cache_stats(slab_cache_t* cache, uint32_t* total_objects, uint32_t* used_objects, uint32_t* free_objects);

#endif /* SLAB_H */