/**
 * gc.c — 简单垃圾回收器
 *
 * 实现标记-清除 (Mark-Sweep) 算法。
 *
 * 设计要点:
 *   - 保守式 GC: 将所有看起来像指针的值当作引用
 *   - 停世界 (Stop-The-World): GC 期间暂停所有 Java 线程
 *   - 对象头: 每个对象前有 GC 元数据
 *
 * 后续优化方向:
 *   - 分代回收 (Generational)
 *   - 增量标记 (Incremental Mark)
 *   - 压缩 (Compaction)
 */
#include "jvm.h"
#include "os_port.h"

/* ================================================================
 * 对象头
 * ================================================================ */

/** 每个 Java 对象的 GC 头部 */
typedef struct gc_header_s {
    uint32_t  size;          /* 对象总大小（含头部） */
    uint32_t  flags;         /* 标志位 */
    struct gc_header_s *next;/* 对象链表 */
} gc_header_t;

#define GC_FLAG_MARKED   0x01   /* 标记位 */
#define GC_FLAG_ARRAY    0x02   /* 是数组 */

/** 获取对象头 */
static inline gc_header_t *gc_hdr(j_ref obj) {
    return ((gc_header_t *)obj) - 1;
}

/** 从对象头获取对象体 */
static inline j_ref gc_body(gc_header_t *hdr) {
    return (j_ref)(hdr + 1);
}

/* ================================================================
 * GC 全局状态
 * ================================================================ */

typedef struct {
    gc_header_t *obj_list;       /* 所有已分配对象的链表 */
    uint32_t     obj_count;      /* 对象计数 */
    uint32_t     heap_used;      /* 堆使用量 */
    uint32_t     gc_threshold;   /* 触发 GC 的阈值 */
    uint8_t      gc_running;     /* GC 是否正在运行 */

    /* 根集合 */
    jvm_frame_t  **roots;
    uint32_t        root_count;
} gc_state_t;

static gc_state_t gc;

/* ================================================================
 * 初始化
 * ================================================================ */

void gc_init(uint32_t heap_size) {
    gc.obj_list    = NULL;
    gc.obj_count   = 0;
    gc.heap_used   = 0;
    gc.gc_threshold = heap_size / 2; /* 堆使用超过 50% 触发 GC */
    gc.gc_running  = 0;
    gc.roots       = NULL;
    gc.root_count  = 0;
}

/* ================================================================
 * 内存分配
 * ================================================================ */

/**
 * 分配 Java 对象
 *
 * @param size  对象体大小（不含头部）
 * @return 对象引用，失败返回 NULL
 */
j_ref gc_alloc(uint32_t size) {
    /* 检查是否需要 GC */
    if (gc.heap_used + size > gc.gc_threshold) {
        gc_collect();
    }

    /* 分配对象（头部 + 体） */
    uint32_t total = sizeof(gc_header_t) + size;
    gc_header_t *hdr = (gc_header_t *)os_malloc(total);
    if (!hdr) {
        /* 分配失败，尝试 GC 后重试 */
        gc_collect();
        hdr = (gc_header_t *)os_malloc(total);
        if (!hdr) {
            os_print("[GC] ERROR: Out of memory\n");
            return NULL;
        }
    }

    hdr->size  = total;
    hdr->flags = 0;
    hdr->next  = gc.obj_list;
    gc.obj_list = hdr;
    gc.obj_count++;
    gc.heap_used += total;

    /* 清零对象体 */
    j_ref obj = gc_body(hdr);
    uint8_t *p = (uint8_t *)obj;
    for (uint32_t i = 0; i < size; i++) {
        p[i] = 0;
    }

    return obj;
}

/**
 * 分配 Java 数组
 */
j_array gc_alloc_array(uint32_t element_size, uint32_t count) {
    /* 数组头部额外存储长度 */
    uint32_t size = 4 + element_size * count; /* 4 字节存长度 */
    j_ref arr = gc_alloc(size);
    if (arr) {
        gc_hdr(arr)->flags |= GC_FLAG_ARRAY;
        *((uint32_t *)arr) = count; /* 前 4 字节存数组长度 */
    }
    return arr;
}

/* ================================================================
 * 标记阶段
 * ================================================================ */

/** 标记一个对象及其引用的所有对象 */
static void mark_object(j_ref obj) {
    if (!obj) return;

    gc_header_t *hdr = gc_hdr(obj);
    if (hdr->flags & GC_FLAG_MARKED) return; /* 已标记 */

    hdr->flags |= GC_FLAG_MARKED;

    /* 递归扫描对象中的引用字段 */
    /* TODO: 需要类信息来确定哪些偏移是引用类型 */
    /* 保守式: 扫描整个对象 */
    uint32_t body_size = hdr->size - sizeof(gc_header_t);
    uint32_t *words = (uint32_t *)obj;
    uint32_t word_count = body_size / 4;

    for (uint32_t i = 0; i < word_count; i++) {
        uint32_t val = words[i];
        /* 保守检查: 值是否在堆范围内 */
        if (val >= (uint32_t)gc_body(NULL) &&
            val <  (uint32_t)gc_body(NULL) + gc.heap_used) {
            mark_object((j_ref)val);
        }
    }
}

/** 标记根集合 */
static void mark_roots(void) {
    for (uint32_t i = 0; i < gc.root_count; i++) {
        jvm_frame_t *frame = gc.roots[i];
        while (frame) {
            /* 扫描局部变量 */
            for (uint16_t j = 0; j < frame->locals_count; j++) {
                mark_object((j_ref)(uint32_t)frame->locals[j]);
            }
            /* 扫描操作数栈 */
            for (uint32_t j = 0; j < frame->stack_top; j++) {
                mark_object((j_ref)(uint32_t)frame->stack[j]);
            }
            frame = frame->prev;
        }
    }
}

/* ================================================================
 * 清除阶段
 * ================================================================ */

static void sweep(void) {
    gc_header_t **prev = &gc.obj_list;
    gc_header_t *curr = gc.obj_list;

    while (curr) {
        if (curr->flags & GC_FLAG_MARKED) {
            /* 存活对象: 清除标记位（为下次 GC 准备） */
            curr->flags &= ~GC_FLAG_MARKED;
            prev = &curr->next;
            curr = curr->next;
        } else {
            /* 死对象: 从链表中移除并释放 */
            gc_header_t *dead = curr;
            *prev = curr->next;
            curr = curr->next;
            gc.heap_used -= dead->size;
            gc.obj_count--;
            os_free(dead);
        }
    }
}

/* ================================================================
 * GC 入口
 * ================================================================ */

/**
 * 执行一次完整的 GC 周期
 */
void gc_collect(void) {
    if (gc.gc_running) return;
    gc.gc_running = 1;

    os_print("[GC] Starting collection...\n");
    uint32_t before = gc.obj_count;

    /* 1. 清除所有标记 */
    gc_header_t *curr = gc.obj_list;
    while (curr) {
        curr->flags &= ~GC_FLAG_MARKED;
        curr = curr->next;
    }

    /* 2. 从根集合开始标记 */
    mark_roots();

    /* 3. 清除未标记的对象 */
    sweep();

    os_print("[GC] Collection complete: ");
    /* TODO: print before - gc.obj_count */
    os_print(" objects freed\n");

    gc.gc_running = 0;
}

/**
 * 请求 GC（异步——在当前分配失败时改为同步调用）
 */
void gc_request(void) {
    gc_collect();
}
