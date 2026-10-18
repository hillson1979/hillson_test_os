#ifndef MM_USER_VM_H
#define MM_USER_VM_H

#include "types.h"
#include "task.h"

/* User address-space primitives. Physical pages come from the buddy PMM;
 * this layer owns only the process-CR3 mapping policy. */
int user_vm_map_page(task_t *task, uint32_t va, uint32_t pa, uint32_t flags);
uint32_t user_vm_alloc_page(task_t *task, uint32_t va, uint32_t flags);

#endif
