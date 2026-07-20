/**
 * @file desktop.cpp
 * @brief HillsonOS Desktop — main entry point
 *
 * Graphical desktop with wallpaper, taskbar, clock, icons,
 * window management, text editor, and USB monitor.
 */
#include "qpainter.h"
#include "qdesktop.h"
#include "qdesktopwindow.h"
#include "qtexteditorapp.h"
#include "qusbmonitor.h"
#include "qsysinfo.h"

extern "C" {
#include "libuser_minimal.h"
}

#define SYS_GUI_INPUT_READ 72
#define SYS_YIELD 3

typedef struct { uint32_t type; int x; int y; uint32_t pressed; } input_event_t;

static int read_input(input_event_t *ev, int type) {
    int r;
    __asm__ volatile("int $0x80":"=a"(r):"a"(SYS_GUI_INPUT_READ),"b"(ev),"c"(type):"memory","cc");
    return r;
}
static void yield_cpu() {
    __asm__ volatile("int $0x80"::"a"(SYS_YIELD):"memory","cc");
}

// Icon click callbacks
static QDesktop *g_desktop = nullptr;

static void launchEditor(void *userData) {
    if (!g_desktop) return;
    createTextEditorApp(g_desktop);
}

static void launchUsbMonitor(void *userData) {
    if (!g_desktop) return;
    createUsbMonitorApp(g_desktop);
}

static void launchSysInfo(void *userData) {
    if (!g_desktop) return;
    createSysInfoApp(g_desktop);
}

extern "C" void usbmon_refresh(void *w);
extern "C" void sysinfo_refresh(void *w);

int g_lastScancode = 0;  // for qdesktop debug display

int main(void) {
    // Get framebuffer info
    fb_info_t fb;
    if (gui_get_fb_info(&fb) != 0) {
        printf("DESKTOP: Failed to get framebuffer info!\n");
        return -1;
    }

    uint32_t *fb_virt = (uint32_t*)0xF0000000;
    // Guard: ensure fb is mapped
    if (!fb_virt || fb.width == 0 || fb.height == 0) {
        printf("DESKTOP: Invalid framebuffer!\n");
        return -1;
    }

    QPainter painter(fb_virt, fb.width, fb.height, fb.pitch);

    // Create desktop
    QDesktop desktop(fb.width, fb.height);
    g_desktop = &desktop;

    // Add desktop icons
    desktop.addIcon(20, 20,  "Editor",  0x004080C0, launchEditor,    nullptr);
    desktop.addIcon(20, 110, "USB Mon", 0x0040A060, launchUsbMonitor, nullptr);
    desktop.addIcon(20, 200, "SysInfo", 0x00804040, launchSysInfo,   nullptr);

    // Initial render
    desktop.render(&painter);

    // Mouse cursor state
    int mx = fb.width / 2, my = fb.height / 2;
    int lcx = -1, lcy = -1;
    int pp = fb.pitch / 4;

    // XOR cursor drawing helper
    auto drawCursor = [&](int cx, int cy) {
        for (int dy = -8; dy <= 8; dy++)
            if (cy + dy >= 0 && cy + dy < (int)fb.height)
                fb_virt[(cy + dy) * pp + cx] ^= 0x00FFFFFF;
        for (int dx = -8; dx <= 8; dx++)
            if (cx + dx >= 0 && cx + dx < (int)fb.width)
                fb_virt[cy * pp + (cx + dx)] ^= 0x00FFFFFF;
    };

    // Initial cursor
    drawCursor(mx, my);
    lcx = mx; lcy = my;

    // Keyboard state
    bool shift = false;
    bool e0prefix = false;
    bool f0release = false;

    // Main event loop
    while (1) {
        yield_cpu();

        bool needRender = false;

        // Read mouse
        input_event_t me;
        int mr = read_input(&me, 2);
        if (mr == 1) {
            // Erase old cursor
            if (lcx >= 0 && lcx < (int)fb.width && lcy >= 0 && lcy < (int)fb.height) {
                drawCursor(lcx, lcy);
            }

            mx = me.x; my = me.y;
            if (mx < 0) mx = 0;
            if (my < 0) my = 0;
            if (mx >= (int)fb.width) mx = fb.width - 1;
            if (my >= (int)fb.height) my = fb.height - 1;

            // Handle mouse press AND release (for drag start/end)
            // me.pressed now encodes buttons as bitmask: bit0=L, bit1=R, bit2=M
            desktop.handleMouse(mx, my, me.pressed, &needRender);

            // Update drag on any mouse move while dragging
            if (desktop.isDragging()) {
                desktop.updateDrag(mx, my);
                needRender = true;
            }

            // Draw new cursor
            drawCursor(mx, my);
            lcx = mx; lcy = my;
        }

        // Read keyboard
        input_event_t ke;
        int kr = read_input(&ke, 1);
        if (kr == 1) {
            int sc = ke.x;
            // Handle E0 prefix
            if (sc == 0xE0) {
                e0prefix = true;
                continue;
            }
            if (e0prefix) {
                sc |= 0xE000;
                e0prefix = false;
            }

            // Track shift state (S1: 2A/36 make, AA/B6 break; S2: 12/59 make, F0 12/F0 59 break)
            if (sc == 0x12 || sc == 0x59) shift = true;   // Set2 LShift/RShift make
            if (sc == 0x2A || sc == 0x36) shift = true;   // Set1 LShift/RShift make
            if (sc == 0xAA || sc == 0xB6) shift = false;  // Set1 break

            // Handle key press (not release)
            // Release codes have bit 7 set in Set1, 0xF0 prefix in Set2
            bool isRelease = (sc & 0x80) || (sc == 0xF0);
            if (sc == 0xF0) { f0release = true; continue; }
            if (f0release) { f0release = false; continue; } // skip Set2 release byte

            if (!isRelease && !(sc & 0x80)) {
                // Show scancode for debugging
                g_lastScancode = sc;

                // Strip E0 prefix for raw key identification
                int rawSc = (sc & 0x7F);
                if (sc & 0xE000) rawSc = sc & 0xFF; // extended key, use low byte

                // ===== Global shortcuts (Set1 | Set2) =====
                // ESC (S1:0x01, S2:0x76)
                if (rawSc == 0x01 || rawSc == 0x76) {
                    if (desktop.windowCount() == 0) break;
                    QDesktopWindow *fw = desktop.focusedWindow();
                    if (fw) {
                        if (lcx >= 0 && lcx < (int)fb.width && lcy >= 0 && lcy < (int)fb.height)
                            drawCursor(lcx, lcy);
                        desktop.removeWindow(fw);
                        delete fw;
                        needRender = true;
                        drawCursor(mx, my);
                        lcx = mx; lcy = my;
                    }
                    continue;
                }

                // Tab (S1:0x0F, S2:0x0D)
                if (rawSc == 0x0F || rawSc == 0x0D) {
                    if (desktop.windowCount() > 1) {
                        QDesktopWindow *cur = desktop.focusedWindow();
                        int ci = -1;
                        for (int i = 0; i < desktop.windowCount(); i++) {
                            if (desktop.window(i) == cur) { ci = i; break; }
                        }
                        int ni = (ci + 1) % desktop.windowCount();
                        desktop.focusWindow(desktop.window(ni));
                        needRender = true;
                    }
                    continue;
                }

                // Arrow keys (S1: 48/50/4B/4D,  S2: 75/72/6B/74)
                int arrowStep = 20;
                bool cursorMoved = false;
                if (rawSc == 0x4B || rawSc == 0x6B) { mx -= arrowStep; cursorMoved = true; } // Left
                if (rawSc == 0x4D || rawSc == 0x74) { mx += arrowStep; cursorMoved = true; } // Right
                if (rawSc == 0x48 || rawSc == 0x75) { my -= arrowStep; cursorMoved = true; } // Up
                if (rawSc == 0x50 || rawSc == 0x72) { my += arrowStep; cursorMoved = true; } // Down
                if (cursorMoved) {
                    if (mx < 0) mx = 0; if (my < 0) my = 0;
                    if (mx >= (int)fb.width) mx = fb.width - 1;
                    if (my >= (int)fb.height) my = fb.height - 1;
                    if (lcx >= 0 && lcx < (int)fb.width && lcy >= 0 && lcy < (int)fb.height)
                        drawCursor(lcx, lcy);
                    drawCursor(mx, my);
                    lcx = mx; lcy = my;
                    if (desktop.isDragging()) {
                        desktop.updateDrag(mx, my);
                        needRender = true;
                    }
                    continue;
                }

                // Enter (S1:0x1C, S2:0x5A) / Space (S1:0x39, S2:0x29)
                if (rawSc == 0x1C || rawSc == 0x5A || rawSc == 0x39 || rawSc == 0x29) {
                    if (lcx >= 0 && lcx < (int)fb.width && lcy >= 0 && lcy < (int)fb.height)
                        drawCursor(lcx, lcy);
                    desktop.handleMouse(mx, my, 1, &needRender);  // press
                    desktop.handleMouse(mx, my, 0, &needRender);  // release
                    drawCursor(mx, my);
                    lcx = mx; lcy = my;
                    continue;
                }

                // F5 (S1:0x3F, S2:0x03)
                if (rawSc == 0x3F || rawSc == 0x03) {
                    QDesktopWindow *fw = desktop.focusedWindow();
                    if (fw && fw->content()) {
                        const char *cn = fw->content()->className();
                        // QUsbMonitor or QSysInfo — call refresh
                        if (cn[0] == 'Q' && cn[1] == 'U') // QUsbMonitor
                            { usbmon_refresh(fw->content()); }
                        if (cn[0] == 'Q' && cn[1] == 'S') // QSysInfo
                            { sysinfo_refresh(fw->content()); }
                        needRender = true;
                    }
                    continue;
                }

                // Forward to focused window for text input
                desktop.handleKey(sc, shift, &needRender);
            }
        }

        // Render if needed
        if (needRender) {
            // Erase cursor before render
            if (lcx >= 0 && lcx < (int)fb.width && lcy >= 0 && lcy < (int)fb.height) {
                drawCursor(lcx, lcy);
            }
            desktop.render(&painter);
            // Redraw cursor
            drawCursor(mx, my);
            lcx = mx; lcy = my;
        }
    }

    return 0;
}
