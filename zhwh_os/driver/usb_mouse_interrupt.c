/**
 * @file usb_mouse_interrupt.c
 * @brief USB鼠标中断模式驱动
 *
 * 使用UHCI中断来接收鼠标数据,而不是轮询
 */

#include "types.h"
#include "usb.h"
#include "usb_hcd.h"
#include "stdio.h"
#include "string.h"

// 中断模式鼠标状态
static struct {
    volatile int data_ready;      // 数据就绪标志
    uint8_t last_report[8];      // 最后一次鼠标报告
    int active;                  // 是否已激活
} int_mouse = {0};

/**
 * @brief UHCI中断处理函数 - 处理鼠标数据
 *
 * 当UHCI控制器产生中断时调用(IOC位设置)
 */
void usb_mouse_irq_handler(void) {
    if (!int_mouse.active) {
        return;
    }

    // TODO: 从UHCI TD读取鼠标数据并保存到last_report
    // 设置data_ready标志

    int_mouse.data_ready = 1;
}

/**
 * @brief 初始化中断模式鼠标
 */
int usb_mouse_interrupt_init(int controller_id, uint8_t dev_addr, uint8_t ep, int low_speed) {
    int_mouse.active = 1;
    int_mouse.data_ready = 0;
    memset(int_mouse.last_report, 0, sizeof(int_mouse.last_report));

    printf("[USB Mouse Interrupt] Initialized (dev=%d ep=%d)\n", dev_addr, ep);
    return 0;
}

/**
 * @brief 读取鼠标数据(非阻塞)
 *
 * @param report 输出缓冲区(至少8字节)
 * @return 读取的字节数,0表示无数据
 */
int usb_mouse_interrupt_poll(uint8_t *report) {
    if (!int_mouse.active) {
        return 0;
    }

    if (!report) {
        return -1;
    }

    // 检查是否有新数据
    if (!int_mouse.data_ready) {
        return 0;
    }

    // 复制数据到用户缓冲区
    memcpy(report, int_mouse.last_report, 3);

    // 调试输出
    static int poll_count = 0;
    if (++poll_count <= 10) {
        printf("[USB Mouse IRQ] POLL #%d: btn=%d x=%d y=%d\n",
               poll_count, report[0] & 0x07, (int8_t)report[1], (int8_t)report[2]);
    }

    // 清除标志
    int_mouse.data_ready = 0;

    return 3;
}
