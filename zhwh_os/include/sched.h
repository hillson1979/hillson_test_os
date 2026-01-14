#include "types.h"
//#include "task.h"


// 静态辅助函数声明
int calculate_weight(int nice);
static uint64_t calculate_inverse_weight(int weight);
static void update_vruntime(struct task_t *task);
static struct task_t *pick_next_task_cfs(void);

// ⚠️ switch_to 现在是汇编实现 (task_impl.s)，不是 inline 函数
// 原因：需要完整的寄存器保存/恢复 (EBP, EDI, ESI, EBX)
// C 的 inline 版本无法正确处理栈切换
extern void switch_to(struct task_t *prev, struct task_t *next);

void efficient_scheduler_loop();


