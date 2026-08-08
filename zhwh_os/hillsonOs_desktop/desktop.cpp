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
#include "include/qkbdhillson_qws.h"
#include "include/qnamespace_qt.h"

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

// Qt keyboard handler — replaces manual scancode parsing
static QHillsonKeyboardHandler g_kbd;

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
    uint32_t cursorBg[25 * 25];
    bool cursorVisible = false;
    int cursorBgX = 0, cursorBgY = 0;

    auto saveCursorBg = [&](int cx, int cy) {
        cursorBgX = cx;
        cursorBgY = cy;
        for (int dy = -12; dy <= 12; dy++) {
            for (int dx = -12; dx <= 12; dx++) {
                int x = cx + dx;
                int y = cy + dy;
                int i = (dy + 12) * 25 + (dx + 12);
                if (x >= 0 && x < (int)fb.width && y >= 0 && y < (int)fb.height)
                    cursorBg[i] = fb_virt[y * pp + x];
                else
                    cursorBg[i] = 0;
            }
        }
    };

    auto restoreCursorBg = [&]() {
        if (!cursorVisible)
            return;
        for (int dy = -12; dy <= 12; dy++) {
            for (int dx = -12; dx <= 12; dx++) {
                int x = cursorBgX + dx;
                int y = cursorBgY + dy;
                int i = (dy + 12) * 25 + (dx + 12);
                if (x >= 0 && x < (int)fb.width && y >= 0 && y < (int)fb.height)
                    fb_virt[y * pp + x] = cursorBg[i];
            }
        }
        cursorVisible = false;
    };

    auto drawCursor = [&](int cx, int cy) {
        saveCursorBg(cx, cy);
        for (int dy = -12; dy <= 12; dy++)
            if (cy + dy >= 0 && cy + dy < (int)fb.height)
                fb_virt[(cy + dy) * pp + cx] = 0x00FFFFFF;
        for (int dx = -12; dx <= 12; dx++)
            if (cx + dx >= 0 && cx + dx < (int)fb.width)
                fb_virt[cy * pp + (cx + dx)] = 0x00FFFFFF;
        // filled center dot
        for (int dy = -2; dy <= 2; dy++)
            for (int dx = -2; dx <= 2; dx++)
                if (cy+dy >= 0 && cy+dy < (int)fb.height && cx+dx >= 0 && cx+dx < (int)fb.width)
                    fb_virt[(cy+dy)*pp + cx+dx] = 0x00FFFF00;
        cursorVisible = true;
    };

    // Initial cursor
    drawCursor(mx, my);
    lcx = mx; lcy = my;

    // Main event loop
    while (1) {
        yield_cpu();

        bool needRender = false;

        // Read mouse
        input_event_t me;
        int mr = read_input(&me, 2);
        if (mr == 1) {
            // Erase old cursor by restoring the saved framebuffer pixels.
            restoreCursorBg();

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

        // Read keyboard via Qt/Embedded handler
        while (g_kbd.poll()) {
            int qtKey  = g_kbd.lastKeyCode();
            int uni    = g_kbd.lastUnicode();
            bool isShift = g_kbd.isShift();
            bool isCtrl  = g_kbd.isCtrl();
            bool isAlt   = g_kbd.isAlt();
            g_lastScancode = qtKey;  // show Qt key in debug

            // ===== Global shortcuts =====
            // ESC — close focused window
            if (qtKey == Qt::Key_Escape) {
                if (desktop.windowCount() > 0) {
                    QDesktopWindow *fw = desktop.focusedWindow();
                    if (fw) {
                        restoreCursorBg();
                        desktop.removeWindow(fw);
                        delete fw;
                        needRender = true;
                        drawCursor(mx, my);
                        lcx = mx; lcy = my;
                    }
                }
                continue;
            }

            // Check if focused window is terminal
            bool isTerm = false;
            QDesktopWindow *fw = desktop.focusedWindow();
            if (fw && fw->content()) {
                const char *cn = fw->content()->className();
                if (cn[0]=='Q' && cn[1]=='T' && cn[2]=='e' && cn[3]=='r') isTerm = true;
            }

            // Tab — switch window
            if (qtKey == Qt::Key_Tab) {
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

            // Arrow keys — move cursor (unless terminal focused)
            if (!isTerm) {
                int arrowStep = 40;
                bool cursorMoved = false;
                if (qtKey == Qt::Key_Left)  { mx -= arrowStep; cursorMoved = true; }
                if (qtKey == Qt::Key_Right) { mx += arrowStep; cursorMoved = true; }
                if (qtKey == Qt::Key_Up)    { my -= arrowStep; cursorMoved = true; }
                if (qtKey == Qt::Key_Down)  { my += arrowStep; cursorMoved = true; }
                if (cursorMoved) {
                    if (mx < 0) mx = 0; if (my < 0) my = 0;
                    if (mx >= (int)fb.width) mx = fb.width - 1;
                    if (my >= (int)fb.height) my = fb.height - 1;
                    restoreCursorBg();
                    drawCursor(mx, my);
                    lcx = mx; lcy = my;
                    if (desktop.isDragging()) {
                        desktop.updateDrag(mx, my);
                        needRender = true;
                    }
                    continue;
                }
            }

            // Enter / Space — click (unless terminal focused)
            if (!isTerm && (qtKey == Qt::Key_Return || qtKey == Qt::Key_Enter || qtKey == Qt::Key_Space)) {
                restoreCursorBg();
                desktop.handleMouse(mx, my, 1, &needRender);
                desktop.handleMouse(mx, my, 0, &needRender);
                drawCursor(mx, my);
                lcx = mx; lcy = my;
                continue;
            }

            // F1 — force repaint
            if (qtKey == Qt::Key_F1) {
                needRender = true;
                continue;
            }

            // F5 — refresh
            if (qtKey == Qt::Key_F5) {
                QDesktopWindow *fww = desktop.focusedWindow();
                if (fww && fww->content()) {
                    const char *cnn = fww->content()->className();
                    if (cnn[0] == 'Q' && cnn[1] == 'U')
                        { usbmon_refresh(fww->content()); }
                    if (cnn[0] == 'Q' && cnn[1] == 'S')
                        { sysinfo_refresh(fww->content()); }
                    needRender = true;
                }
                continue;
            }

            // Forward to focused window for text input
            // Pass Qt key code + Unicode char to the widget
            desktop.handleQtKey(qtKey, uni, isShift, &needRender);
        }

        // Render if needed
        if (needRender) {
            restoreCursorBg();
            desktop.render(&painter);
            // Redraw cursor
            drawCursor(mx, my);
            lcx = mx; lcy = my;
        }
    }

    return 0;
}
