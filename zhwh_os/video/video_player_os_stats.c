/**
 * @file video_player_os_stats.c
 * @brief 视频播放器系统统计接口实现
 *
 * 这些函数需要根据你的OS实现
 * 这里提供的是简化版本（返回默认值）
 */

#include "video_player.h"

/**
 * @brief 获取内存使用量
 *
 * @return uint32_t 已使用的内存（bytes）
 *
 * TODO: 根据你的OS内存管理器实现
 */
uint32_t get_memory_usage(void) {
    // 示例：如果你的OS有 malloc_stats
    #ifdef HAS_MALLOC_STATS
    extern size_t malloc_used_memory(void);
    return malloc_used_memory();
    #else
    // 返回0表示不支持
    return 0;
    #endif
}

/**
 * @brief 获取CPU占用率
 *
 * @return uint32_t CPU占用率（千分比，0-1000）
 *
 * TODO: 根据你的OS调度器实现
 *
 * 示例实现思路：
 * 1. 记录视频任务运行时间
 * 2. 记录总CPU时间
 * 3. 计算比例
 */
uint32_t get_cpu_usage(void) {
    // 示例：如果你的OS有 CPU 统计
    #ifdef HAS_CPU_STATS
    extern uint32_t os_get_cpu_usage(void);
    return os_get_cpu_usage();
    #else
    // 返回0表示不支持
    return 0;
    #endif
}

/**
 * @brief 获取网络带宽
 *
 * @return uint32_t 当前带宽（bytes/s）
 *
 * 这个已经在 video_player.c 中实现了
 * 只需要确保 net_recv_frame 正确统计字节数
 */

/**
 * @brief 扩展：为你的OS添加系统统计
 *
 * 方法1：添加全局计数器
 *   static uint64_t total_memory_allocated = 0;
 *   static uint32_t video_task_cpu_time = 0;
 *
 * 方法2：使用性能计数器（如果CPU支持）
 *   - 读取 TSC (Time Stamp Counter)
 *   - 计算 CPU 周期数
 *
 * 方法3：使用定时器中断
 *   - 定期采样哪个任务在运行
 *   - 统计时间分配
 */
