#include "types.h"
//#include "task.h"


// 静态辅助函数声明
int calculate_weight(int nice);
static uint64_t calculate_inverse_weight(int weight);
static void update_vruntime(struct task_t *task);
static struct task_t *pick_next_task_cfs(void);

static inline void switch_to(struct task_t *prev,struct task_t *next);

void efficient_scheduler_loop();


