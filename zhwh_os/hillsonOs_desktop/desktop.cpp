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
#include "qterminal.h"

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

static void launchTerminal(void *userData) {
    if (!g_desktop) return;
    createTerminalApp(g_desktop);
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
    desktop.addIcon(20, 20,  "Term",    0x0040A0A0, launchTerminal,   nullptr);
    desktop.addIcon(20, 110, "Editor",  0x004080C0, launchEditor,    nullptr);
    desktop.addIcon(20, 200, "USB Mon", 0x0040A060, launchUsbMonitor, nullptr);
    desktop.addIcon(20, 290, "SysInfo", 0x00804040, launchSysInfo,   nullptr);

    // Auto-open Terminal as default window
    createTerminalApp(&desktop);

    // Initial render
    desktop.render(&painter);

    // Mouse cursor state
    int mx = fb.width / 2, my = fb.height / 2;
    int lcx = -1, lcy = -1;
    int pp = fb.pitch / 4;

    // XOR cursor drawing helper (big crosshair)
    auto drawCursor = [&](int cx, int cy) {
        for (int dy = -12; dy <= 12; dy++)
            if (cy + dy >= 0 && cy + dy < (int)fb.height)
                fb_virt[(cy + dy) * pp + cx] ^= 0x00FFFFFF;
        for (int dx = -12; dx <= 12; dx++)
            if (cx + dx >= 0 && cx + dx < (int)fb.width)
                fb_virt[cy * pp + (cx + dx)] ^= 0x00FFFFFF;
        // filled center dot
        for (int dy = -2; dy <= 2; dy++)
            for (int dx = -2; dx <= 2; dx++)
                if (cy+dy >= 0 && cy+dy < (int)fb.height && cx+dx >= 0 && cx+dx < (int)fb.width)
                    fb_virt[(cy+dy)*pp + cx+dx] ^= 0x00FFFF00;
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

                // Check if focused window is terminal (needs raw key events)
                bool isTerm = false;
                QDesktopWindow *fw = desktop.focusedWindow();
                if (fw && fw->content()) {
                    const char *cn = fw->content()->className();
                    if (cn[0]=='Q' && cn[1]=='T' && cn[2]=='e' && cn[3]=='r') isTerm = true;
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

                // Arrow keys: move cursor (unless terminal focused)
                if (!isTerm) {
                    int arrowStep = 40;
                    bool cursorMoved = false;
                    if (rawSc == 0x4B || rawSc == 0x6B) { mx -= arrowStep; cursorMoved = true; }
                    if (rawSc == 0x4D || rawSc == 0x74) { mx += arrowStep; cursorMoved = true; }
                    if (rawSc == 0x48 || rawSc == 0x75) { my -= arrowStep; cursorMoved = true; }
                    if (rawSc == 0x50 || rawSc == 0x72) { my += arrowStep; cursorMoved = true; }
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
                }

                // Enter / Space: click (unless terminal focused)
                if (!isTerm && (rawSc == 0x1C || rawSc == 0x5A || rawSc == 0x39 || rawSc == 0x29)) {
                    if (lcx >= 0 && lcx < (int)fb.width && lcy >= 0 && lcy < (int)fb.height)
                        drawCursor(lcx, lcy);
                    desktop.handleMouse(mx, my, 1, &needRender);
                    desktop.handleMouse(mx, my, 0, &needRender);
                    drawCursor(mx, my);
                    lcx = mx; lcy = my;
                    continue;
                }

                // F1 (S1:0x3B, S2:0x05) — toggle debug overlay, force repaint
                if (rawSc == 0x3B || rawSc == 0x05) {
                    needRender = true;
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
            // Only erase cursor if it has moved (avoid flicker on keyboard events)
            bool cursorMovedRender = (mx != lcx || my != lcy);
            if (cursorMovedRender && lcx >= 0 && lcx < (int)fb.width && lcy >= 0 && lcy < (int)fb.height) {
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
