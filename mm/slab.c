#include "mm/slab.h"
#include "mm/buddy.h"
#include "printf.h"
#include "string.h"
#include "page.h"

#define PAGE_SIZE 4096

// Slab 管理结构
static slab_cache_t* kmem_cache;  // 用于分配 slab 结构的缓存

// 计算每个 slab 中的对象数量
static uint32_t calculate_num_per_slab(uint32_t obj_size, uint32_t align) {
    uint32_t total_size = sizeof(slab_t);
    uint32_t num = 0;
    
    // 计算第一个对象的对齐地址
    char* start = (char*)1;  // 临时地址
    char* aligned_start = (char*)((uint32_t)(start + sizeof(slab_t) + align - 1) & ~(align - 1));
    uint32_t offset = aligned_start - start;
    
    // 计算可以容纳的对象数量
    while (total_size + offset + num * obj_size <= PAGE_SIZE) {
        num++;
    }
    
    if (num > 0) {
        num--;
    }
    
    return num;
}

// 创建新的 slab
static slab_t* create_slab(slab_cache_t* cache) {
    uint32_t page, i;
    slab_t* slab;
    char* start;
    
    // 从 buddy system 分配一页
    page = buddy_alloc(0);  // order 0 = 1 page
    if (!page) {
        return NULL;
    }
    
    // 映射到虚拟地址
    start = phys_to_virt(page * PAGE_SIZE);
    memset(start, 0, PAGE_SIZE);
    
    slab = (slab_t*)start;
    slab->next = slab->prev = NULL;
    slab->flags = cache->flags;
    slab->free_count = cache->num_per_slab;
    slab->total_count = cache->num_per_slab;
    
    // 计算对象开始地址
    slab->start = (char*)((uint32_t)(start + sizeof(slab_t) + cache->align - 1) & ~(cache->align - 1));
    
    // 分配空闲对象位图
    uint32_t bitmap_size = (cache->num_per_slab + 31) / 32 * sizeof(uint32_t);
    slab->free_objects = (uint32_t*)(slab->start + cache->num_per_slab * cache->object_size);
    
    // 初始化空闲对象位图（所有对象初始化为空闲）
    memset(slab->free_objects, 0, bitmap_size);
    
    // 调用构造函数（如果有）
    if (cache->ctor) {
        for (i = 0; i < cache->num_per_slab; i++) {
            cache->ctor(slab->start + i * cache->object_size);
        }
    }
    
    return slab;
}

// 销毁 slab
static void destroy_slab(slab_cache_t* cache, slab_t* slab) {
    uint32_t i, page;
    
    // 调用析构函数（如果有）
    if (cache->dtor) {
        for (i = 0; i < cache->num_per_slab; i++) {
            if (!(slab->free_objects[i / 32] & (1 << (i % 32)))) {
                cache->dtor(slab->start + i * cache->object_size);
            }
        }
    }
    
    // 计算物理页号并释放
    page = virt_to_phys(slab) / PAGE_SIZE;
    buddy_free(page, 0);
}

// 从链表中移除 slab
static void remove_slab_from_list(slab_t** list, slab_t* slab) {
    if (slab->prev) {
        slab->prev->next = slab->next;
    } else {
        *list = slab->next;
    }
    
    if (slab->next) {
        slab->next->prev = slab->prev;
    }
    
    slab->next = slab->prev = NULL;
}

// 添加 slab 到链表
static void add_slab_to_list(slab_t** list, slab_t* slab) {
    slab->next = *list;
    slab->prev = NULL;
    
    if (*list) {
        (*list)->prev = slab;
    }
    
    *list = slab;
}

// 初始化 slab 分配器
int slab_init(void) {
    // 不创建用于分配 slab 结构的缓存，避免循环依赖
    // 直接使用 kmalloc_early 或静态分配
    printf("slab_init: initialized\n");
    return 0;
}

// 创建 slab 缓存
slab_cache_t* slab_cache_create(const char* name, uint32_t size, uint32_t align, uint32_t flags, 
                               void (*ctor)(void*), void (*dtor)(void*)) {
    slab_cache_t* cache;
    
    // 分配缓存结构 - 这里使用早期的内存分配方式避免循环依赖
    static uint32_t slab_cache_count = 0;
    static slab_cache_t slab_caches[32]; // 静态数组避免循环依赖
    
    if (slab_cache_count >= 32) {
        return NULL;
    }
    
    cache = &slab_caches[slab_cache_count++];
    memset(cache, 0, sizeof(slab_cache_t));
    
    // 设置缓存参数
    cache->name = name;
    cache->object_size = size;
    cache->align = align;
    cache->flags = flags;
    cache->ctor = ctor;
    cache->dtor = dtor;
    
    // 计算每个 slab 中的对象数量
    cache->num_per_slab = calculate_num_per_slab(size, align);
    if (cache->num_per_slab == 0) {
        kfree(cache);
        return NULL;
    }
    
    // 初始化链表
    cache->full_slabs = NULL;
    cache->partial_slabs = NULL;
    cache->empty_slabs = NULL;
    
    // 初始化锁
    spinlock_init(&cache->lock);
    
    printf("slab_cache_create: %s, size=%u, align=%u, num_per_slab=%u\n", 
           name, size, align, cache->num_per_slab);
    
    return cache;
}

// 销毁 slab 缓存
int slab_cache_destroy(slab_cache_t* cache) {
    slab_t* slab, *next;
    
    if (!cache) {
        return -1;
    }
    
    spinlock_acquire(&cache->lock);
    
    // 销毁所有 slab
    // 处理 full slabs
    for (slab = cache->full_slabs; slab; slab = next) {
        next = slab->next;
        destroy_slab(cache, slab);
    }
    
    // 处理 partial slabs
    for (slab = cache->partial_slabs; slab; slab = next) {
        next = slab->next;
        destroy_slab(cache, slab);
    }
    
    // 处理 empty slabs
    for (slab = cache->empty_slabs; slab; slab = next) {
        next = slab->next;
        destroy_slab(cache, slab);
    }
    
    // 重置缓存状态
    cache->full_slabs = NULL;
    cache->partial_slabs = NULL;
    cache->empty_slabs = NULL;
    
    spinlock_release(&cache->lock);
    
    // 因为使用静态数组，不需要释放内存
    return 0;
}

// 从缓存中分配对象
void* slab_alloc(slab_cache_t* cache) {
    slab_t* slab;
    uint32_t i, j;
    void* obj = NULL;
    
    if (!cache) {
        return NULL;
    }
    
    spinlock_acquire(&cache->lock);
    
    // 首先尝试从 partial slabs 中分配
    if (cache->partial_slabs) {
        slab = cache->partial_slabs;
        
        // 查找第一个空闲对象
        for (i = 0; i < slab->total_count; i++) {
            if (!(slab->free_objects[i / 32] & (1 << (i % 32)))) {
                // 标记对象为已使用
                slab->free_objects[i / 32] |= (1 << (i % 32));
                slab->free_count--;
                
                obj = slab->start + i * cache->object_size;
                break;
            }
        }
        
        // 如果 slab 已满，移到 full_slabs
        if (slab->free_count == 0) {
            remove_slab_from_list(&cache->partial_slabs, slab);
            add_slab_to_list(&cache->full_slabs, slab);
        }
    } 
    // 尝试从 empty slabs 中分配
    else if (cache->empty_slabs) {
        slab = cache->empty_slabs;
        remove_slab_from_list(&cache->empty_slabs, slab);
        
        // 标记第一个对象为已使用
        slab->free_objects[0] |= 1;
        slab->free_count = cache->num_per_slab - 1;
        
        obj = slab->start;
        
        // 如果还有空闲对象，移到 partial_slabs
        if (slab->free_count > 0) {
            add_slab_to_list(&cache->partial_slabs, slab);
        } 
        // 否则移到 full_slabs
        else {
            add_slab_to_list(&cache->full_slabs, slab);
        }
    } 
    // 创建新的 slab
    else {
        slab = create_slab(cache);
        if (!slab) {
            spinlock_release(&cache->lock);
            return NULL;
        }
        
        // 标记第一个对象为已使用
        slab->free_objects[0] |= 1;
        slab->free_count = cache->num_per_slab - 1;
        
        obj = slab->start;
        
        // 如果还有空闲对象，移到 partial_slabs
        if (slab->free_count > 0) {
            add_slab_to_list(&cache->partial_slabs, slab);
        } 
        // 否则移到 full_slabs
        else {
            add_slab_to_list(&cache->full_slabs, slab);
        }
    }
    
    spinlock_release(&cache->lock);
    
    // 如果有构造函数，调用它
    if (obj && cache->ctor) {
        cache->ctor(obj);
    }
    
    return obj;
}

// 释放对象到缓存
void slab_free(slab_cache_t* cache, void* obj) {
    slab_t* slab;
    char* start, *end;
    uint32_t i;
    
    if (!cache || !obj) {
        return;
    }
    
    // 查找对象所在的 slab
    start = (char*)((uint32_t)obj & ~(PAGE_SIZE - 1));
    end = start + PAGE_SIZE;
    
    // 确保对象在页面范围内
    if (obj < start || obj >= end) {
        return;
    }
    
    slab = (slab_t*)start;
    
    // 计算对象索引
    i = ((char*)obj - slab->start) / cache->object_size;
    if (i >= slab->total_count) {
        return;
    }
    
    spinlock_acquire(&cache->lock);
    
    // 如果对象已经是空闲的，直接返回
    if (!(slab->free_objects[i / 32] & (1 << (i % 32)))) {
        spinlock_release(&cache->lock);
        return;
    }
    
    // 如果有析构函数，调用它
    if (cache->dtor) {
        cache->dtor(obj);
    }
    
    // 标记对象为空闲
    slab->free_objects[i / 32] &= ~(1 << (i % 32));
    slab->free_count++;
    
    // 检查 slab 当前所在的链表
    if (slab->free_count == slab->total_count) {
        // 如果 slab 现在为空，从当前链表移除并添加到 empty_slabs
        if (slab->prev || cache->full_slabs == slab) {
            remove_slab_from_list(&cache->full_slabs, slab);
        } else {
            remove_slab_from_list(&cache->partial_slabs, slab);
        }
        add_slab_to_list(&cache->empty_slabs, slab);
    } else if (slab->free_count == 1) {
        // 如果 slab 现在是部分满的，从 full_slabs 移除并添加到 partial_slabs
        remove_slab_from_list(&cache->full_slabs, slab);
        add_slab_to_list(&cache->partial_slabs, slab);
    }
    
    spinlock_release(&cache->lock);
}

// 重新调整缓存大小
int slab_cache_resize(slab_cache_t* cache, uint32_t new_size) {
    // 这个功能比较复杂，暂时不实现
    return -1;
}

// 获取缓存统计信息
void slab_cache_stats(slab_cache_t* cache, uint32_t* total_objects, uint32_t* used_objects, uint32_t* free_objects) {
    slab_t* slab;
    uint32_t total = 0, used = 0, free = 0;
    
    if (!cache) {
        return;
    }
    
    spinlock_acquire(&cache->lock);
    
    // 统计 full_slabs
    for (slab = cache->full_slabs; slab; slab = slab->next) {
        total += slab->total_count;
        used += slab->total_count;
    }
    
    // 统计 partial_slabs
    for (slab = cache->partial_slabs; slab; slab = slab->next) {
        total += slab->total_count;
        used += slab->total_count - slab->free_count;
        free += slab->free_count;
    }
    
    // 统计 empty_slabs
    for (slab = cache->empty_slabs; slab; slab = slab->next) {
        total += slab->total_count;
        free += slab->free_count;
    }
    
    spinlock_release(&cache->lock);
    
    if (total_objects) {
        *total_objects = total;
    }
    if (used_objects) {
        *used_objects = used;
    }
    if (free_objects) {
        *free_objects = free;
    }
}