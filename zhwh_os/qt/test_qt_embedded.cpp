/**
 * @file test_qt_embedded.cpp
 * @brief Test program for Qt/Embedded port on HillsonOS
 *
 * Creates a full-screen QWidget, draws UI elements,
 * handles keyboard input and mouse movement.
 */
#include "include/qapplication_qt.h"
#include "include/qscreenhillsonfb_qws.h"
#include "include/qkbdhillson_qws.h"
#include "include/qmousehillson_qws.h"
#include "include/qnamespace_qt.h"
#include "include/qrect_qt.h"
#include "include/qcolor_qt.h"
#include "qwidget.h"
#include "qpainter.h"

// Global state
static char g_typed[256] = "Type here: ";
static int  g_typeLen = 11;
static int  g_mouseX = 512, g_mouseY = 384;
static int  g_mouseBtn = 0;

// Custom main widget with paint and input handling
class TestWidget : public QWidget {
public:
    TestWidget(QWidget *parent, const char *name)
        : QWidget(parent, name)
    {
        setBackgroundColor(0xFF202020);  // dark gray
    }

    void paintEvent(QPainter *p) override
    {
        int W = width();
        int H = height();

        // Background
        p->setColor(backgroundColor());
        p->fillRect(0, 0, W, H);

        // Title bar area
        p->setColor(0xFF004080);
        p->fillRect(0, 0, W, 40);
        p->setColor(COLOR_WHITE);
        p->drawText(16, 12, "Qt/Embedded 3.3.8b on HillsonOS");

        // Status bar at bottom
        p->setColor(0xFF003060);
        p->fillRect(0, H - 30, W, 30);
        p->setColor(0xFFA0A0A0);
        char status[128];
        int sx = g_mouseX;
        int sy = g_mouseY;
        // Simple sprintf-like formatting
        char *sp = status;
        const char *pre = "Mouse: ";
        while (*pre) *sp++ = *pre++;
        *sp++ = '0' + (sx / 1000); sx %= 1000;
        *sp++ = '0' + (sx / 100);  sx %= 100;
        *sp++ = '0' + (sx / 10);   sx %= 10;
        *sp++ = '0' + sx;
        *sp++ = ',';
        *sp++ = ' ';
        *sp++ = '0' + (sy / 1000); sy %= 1000;
        *sp++ = '0' + (sy / 100);  sy %= 100;
        *sp++ = '0' + (sy / 10);   sy %= 10;
        *sp++ = '0' + sy;
        *sp++ = ' ';
        const char *btn = g_mouseBtn ? "BTN" : "   ";
        while (*btn) *sp++ = *btn++;
        *sp++ = ' ';
        const char *esc = "ESC=quit";
        while (*esc) *sp++ = *esc++;
        *sp = 0;
        p->drawText(16, H - 22, status);

        // Center panel
        int px = 40, py = 60, pw = W - 80, ph = H - 110;
        p->setColor(0xFF303030);
        p->fillRect(px, py, pw, ph);
        p->setColor(0xFF606060);
        p->drawRect(px, py, pw, ph);

        // Title
        p->setColor(0xFF00FF00);
        p->drawText(px + 16, py + 12, "Qt Port Test — Press keys to type, move mouse, ESC to exit");

        // Divider
        p->setColor(0xFF505050);
        p->drawLine(px + 16, py + 36, px + pw - 32, py + 36);

        // Typed text
        p->setColor(COLOR_WHITE);
        p->drawText(px + 16, py + 56, g_typed);

        // Cursor
        int cx = px + 16 + (g_typeLen * 8);
        if (g_typeLen < 80) {
            p->setColor(0xFFFFCC00);
            p->fillRect(cx, py + 56, 8, 12);
        }

        // Keyboard input info
        p->setColor(0xFF808080);
        p->drawText(px + 16, py + 80, "Keyboard: Set1 scancode → Qt::Key → Unicode mapping active");
        p->drawText(px + 16, py + 96, "Drivers: QHillsonFbScreen | QHillsonKeyboardHandler | QHillsonMouseHandler");

        // Mouse cursor (crosshair)
        p->setColor(0xFFFF4444);
        int mx = g_mouseX, my = g_mouseY;
        p->drawLine(mx - 8, my, mx + 8, my);
        p->drawLine(mx, my - 8, mx, my + 8);
    }

    const char *className() const override { return "TestWidget"; }
};

// ============================================================
// Main
// ============================================================

int main()
{
    // Create Qt application
    QApplication app;

    // Create test widget
    TestWidget *test = new TestWidget(0, "test");
    app.setMainWidget(test);

    // Get framebuffer for direct cursor/input handling
    QScreen *screen = app.screen();
    if (!screen) return -1;

    unsigned int *fb = (unsigned int*)screen->base();
    int sw = screen->width();
    int sh = screen->height();
    int pp = screen->linestep() / 4;

    QPainter painter(fb, sw, sh, screen->linestep());

    // Initial render
    test->render(&painter);

    // Simple XOR cursor drawing
    auto xorCursor = [&](int cx, int cy) {
        for (int dy = -10; dy <= 10; dy++) {
            if (cy + dy >= 0 && cy + dy < sh) {
                unsigned int *p = fb + (cy + dy) * pp + cx;
                if (cx >= 0 && cx < sw) *p ^= 0x00FFFFFF;
            }
        }
        for (int dx = -10; dx <= 10; dx++) {
            if (cx + dx >= 0 && cx + dx < sw) {
                unsigned int *p = fb + cy * pp + (cx + dx);
                if (cy >= 0 && cy < sh) *p ^= 0x00FFFFFF;
            }
        }
    };

    int lcx = g_mouseX, lcy = g_mouseY;
    xorCursor(g_mouseX, g_mouseY);

    bool needsRender = true;

    // Main event loop (manual — QApplication::exec is simpler but less interactive)
    while (1) {
        // Yield CPU
        __asm__ volatile("int $0x80" :: "a"(3) : "memory", "cc");

        // Poll keyboard
        QHillsonKeyboardHandler *kbd = app.keyboardHandler();
        while (kbd && kbd->poll()) {
            int kc  = kbd->lastKeyCode();
            int uni = kbd->lastUnicode();

            if (kc == Qt::Key_Escape) return 0;  // quit

            // Add printable characters to buffer
            if (uni >= 32 && uni <= 126 && g_typeLen < 250) {
                g_typed[g_typeLen++] = (char)uni;
                g_typed[g_typeLen] = 0;
                needsRender = true;
            }

            // Backspace
            if (kc == Qt::Key_Backspace && g_typeLen > 11) {
                g_typeLen--;
                g_typed[g_typeLen] = 0;
                needsRender = true;
            }
        }

        // Poll mouse
        QHillsonMouseHandler *mouse = app.mouseHandler();
        if (mouse && mouse->poll(sw, sh)) {
            int nx = mouse->pos()._x();
            int ny = mouse->pos()._y();

            // Erase old cursor
            if (lcx >= 0 && lcx < sw && lcy >= 0 && lcy < sh) {
                xorCursor(lcx, lcy);
            }

            g_mouseX = nx;
            g_mouseY = ny;
            needsRender = true;

            // Draw new cursor
            xorCursor(nx, ny);
            lcx = nx;
            lcy = ny;
        }

        // Re-render
        if (needsRender) {
            // Erase cursor before render
            if (lcx >= 0 && lcx < sw && lcy >= 0 && lcy < sh) {
                xorCursor(lcx, lcy);
            }

            test->render(&painter);

            // Redraw cursor
            xorCursor(lcx, lcy);
            needsRender = false;
        }

        // Track mouse buttons
        static int lastBtn = 0;
        if (g_mouseBtn != lastBtn) {
            lastBtn = g_mouseBtn;
            needsRender = true;
        }
    }

    return 0;
}
