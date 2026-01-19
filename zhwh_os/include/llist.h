#ifndef LLIST_H
#define LLIST_H

#include "types.h"
struct llist_header {
    struct llist_header *next, *prev;
};

typedef struct llist_header llist_header_t;
static struct llist_header *sched_root, *sched_sleep;
static inline void
__llist_add(struct llist_header* elem,
            struct llist_header* prev,
            struct llist_header* next)
{
    next->prev = elem;
    elem->next = next;
    elem->prev = prev;
    prev->next = elem;
}
/**
 * 初始化链表头
 */
static inline void
llist_init_head(struct llist_header* head)
{
    head->next = head;
    head->prev = head;
}
/**
 * list_next - 获取链表中下一个条目
 * @ptr:    当前条目的指针
 * @type:   结构体类型
 * @member: 结构体中list_head成员的名称
 *
 * 返回：下一个条目的指针，如果是链表末尾则返回第一个条目（循环链表）
 */
#define list_next(ptr, type, member) ({                  \
    const typeof(((type *)0)->member) *__mptr = &((ptr)->member); \
    type *__next = list_entry(__mptr->next, type, member); \
    __next; })


// 修改宏定义，统一使用指针方式
#define llist_for_each(pos, n, head, member)                                   \
    for (pos = list_entry((head)->next, typeof(*pos), member),                 \
        n = list_entry(pos->member.next, typeof(*pos), member);                \
         &pos->member != (head);                                               \
         pos = n, n = list_entry(n->member.next, typeof(*n), member))

// 调用时确保传入指针
//llist_for_each(pos, n, &sched_sleep, sleep.sleepers)

static inline void
llist_append(struct llist_header* head, struct llist_header* elem)
{
    __llist_add(elem, head->prev, head);
}

typedef uint32_t ptr_t;
#define offsetof(f, m)          __builtin_offsetof(f, m)

#define container_of(ptr, type, member)                                        \
    ({                                                                         \
        const typeof(((type*)0)->member)* __mptr = (ptr);                      \
        ((ptr_t)ptr != 0UL) ? (type*)((char*)__mptr - offsetof(type, member)) : 0;           \
    })

#define list_entry(ptr, type, member) container_of(ptr, type, member)

/**
 * list_next - get the struct for next entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
//#define list_next(current, type, member) \
       // container_of(((typeof(current))current)->member.next, type, member)
//container_of(current->member.next, type, member)


// 直接使用指针运算，避免复杂的类型检查
#define list_next(current, type, member) \
    ((type *)((char *)((current)->member->next) - offsetof(type, member)))


//static inline int llist_empty(struct llist_header *list);

/**
 * llist_del - 从链表中删除一个条目
 * @elem: 要删除的条目
 */
static inline void
llist_del(struct llist_header* elem)
{
    struct llist_header *prev, *next;
    prev = elem->prev;
    next = elem->next;
    next->prev = prev;
    prev->next = next;
}

/**
 * llist_empty - 检查链表是否为空
 * @head: 链表头
 */
static inline int
llist_empty(struct llist_header *head)
{
    return head->next == head;
}

/**
 * llist_for_each - 遍历链表（简化版）
 * @pos: 当前位置
 * @head: 链表头
 */
#define llist_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * llist_for_each_safe - 安全遍历链表（允许删除）
 * @pos: 当前位置
 * @next: 下一个位置（临时变量）
 * @head: 链表头
 */
#define llist_for_each_safe(pos, next, head) \
    for (pos = (head)->next, next = pos->next; pos != (head); \
         pos = next, next = pos->next)

#endif // LLIST_H
