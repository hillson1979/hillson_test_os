#include "llist.h"

/**
 * list_for_each_entry - 遍历链表
 * @pos:    用作循环游标的类型指针
 * @head:   链表的头节点
 * @member: 结构体中list_head成员的名称
 */
#define list_for_each_entry(pos, head, member)              \
    for (pos = list_entry((head)->next, typeof(*pos), member);  \
         &pos->member != (head);                    \
         pos = list_entry(pos->member.next, typeof(*pos), member))



/**
 * 在链表尾部添加新条目
 */
static inline void llist_add_tail(struct llist_header *new, struct llist_header *head)
{
    struct llist_header *prev = head->prev;
    
    new->next = head;
    new->prev = prev;
    prev->next = new;
    head->prev = new;
}

/**
 * 检查链表是否为空
 * @param list 要检查的链表头
 * @return 为空返回1，非空返回0
 */
 int llist_empty(struct llist_header *list) {
    // 空链表时，哨兵节点的next指向自身
    return (list->next == &list);
}
 void
llist_delete(struct llist_header* elem)
{
    struct llist_header* prev = elem->prev;
    struct llist_header* next = elem->next;
    
    prev->next = next;
    next->prev = prev;
    
    // 可选：清空被删除节点的指针，避免悬空引用
    elem->next = elem;
    elem->prev = elem;
}
