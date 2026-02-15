/**
 * @file lvgl_stubs.c
 * @brief Stub implementations for LVGL extra and theme functions
 *
 * This file provides stub implementations for LVGL functions that are
 * called from core code but are not needed for basic functionality.
 */

#include "libuser_minimal.h"

/**
 * @brief Stub for lv_extra_init()
 * Called by lv_init() when extra components are disabled
 */
void lv_extra_init(void) {
    // Nothing to initialize when LV_USE_EXTRA=0
}

/**
 * @brief Stub for lv_theme_default_init()
 * Called by display driver registration
 */
void *lv_theme_default_init(void *disp, int a, int b, int c, int d) {
    // Return NULL - no default theme when extra disabled
    return (void *)0;
}

/**
 * @brief Stub for lv_theme_default_get()
 */
void *lv_theme_default_get(void) {
    // Return NULL - no default theme when extra disabled
    return (void *)0;
}

/**
 * @brief Stub for lv_theme_default_is_inited()
 */
int lv_theme_default_is_inited(void) {
    // Return 0 (false) - not inited when extra disabled
    return 0;
}
