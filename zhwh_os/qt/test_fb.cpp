/**
 * @file test_fb.cpp
 * @brief Ultra-minimal framebuffer test — diagnose pixel writes
 */
extern "C" {
#include "libuser_minimal.h"
}

int main(void) {
    printf("FB START\n");

    fb_info_t info;
    if (gui_get_fb_info(&info) != 0) {
        printf("FB FAIL: gui_get_fb_info\n");
        return -1;
    }

    printf("FB %dx%d pitch=%d bpp=%d\n", info.width, info.height, info.pitch, info.bpp);

    // Use known virtual address directly
    unsigned char *fb = (unsigned char *)0xF0000000;
    int rowBytes = info.pitch;
    int w = info.width;
    int h = info.height;

    // One pixel at (10,10): red
    unsigned *p = (unsigned *)(fb + 10 * rowBytes + 10 * 4);
    unsigned color = 0x00FF0000;
    *p = color;
    unsigned back = *p;
    printf("PIX1: wrote=0x%x read=0x%x %s\n", color, back, (back==color)?"OK":"FAIL");

    // Another pixel at (20,20): green
    p = (unsigned *)(fb + 20 * rowBytes + 20 * 4);
    color = 0x0000FF00;
    *p = color;
    back = *p;
    printf("PIX2: wrote=0x%x read=0x%x %s\n", color, back, (back==color)?"OK":"FAIL");

    // Clear screen to dark gray using byte writes
    unsigned *pix = (unsigned *)fb;
    int total = (rowBytes / 4) * h;
    unsigned gray = 0x00404040;
    for (int i = 0; i < total; i++) {
        pix[i] = gray;
    }
    printf("CLEAR done\n");

    // Red rect top-left
    for (int y = 0; y < 80; y++) {
        for (int x = 0; x < 80; x++) {
            unsigned *ptr = (unsigned *)(fb + y * rowBytes + x * 4);
            *ptr = 0x00FF0000;
        }
    }
    printf("RED rect done\n");

    // Green rect top-right
    for (int y = 0; y < 80; y++) {
        for (int x = w - 80; x < w; x++) {
            unsigned *ptr = (unsigned *)(fb + y * rowBytes + x * 4);
            *ptr = 0x0000FF00;
        }
    }
    printf("GREEN rect done\n");

    // Blue rect bottom-left
    for (int y = h - 80; y < h; y++) {
        for (int x = 0; x < 80; x++) {
            unsigned *ptr = (unsigned *)(fb + y * rowBytes + x * 4);
            *ptr = 0x000000FF;
        }
    }
    printf("BLUE rect done\n");

    // White rect bottom-right
    for (int y = h - 80; y < h; y++) {
        for (int x = w - 80; x < w; x++) {
            unsigned *ptr = (unsigned *)(fb + y * rowBytes + x * 4);
            *ptr = 0x00FFFFFF;
        }
    }
    printf("WHITE rect done\n");

    printf("FB ALL OK\n");
    while(1) { __asm__ volatile("hlt"); }
    return 0;
}
