/**
 * @file lvgl_os.h
 * @brief LVGL 主头文件包装
 */

#ifndef LVGL_OS_H
#define LVGL_OS_H

// 确保在包含任何 LVGL 头文件之前定义这个宏
#define LV_USE_EXTRA 0

// 包含 stdbool.h 定义 bool 类型（必须在包含任何 LVGL 头文件之前）
#include "stdbool.h"
#include "stdint_compat.h"

// 直接使用 LVGL 主头文件
#include "../../lvgl/source/lvgl.h"

#endif /* LVGL_OS_H */
