/**
 * @file qapplication_qt.cpp
 * @brief QApplication implementation for HillsonOS
 *
 * Initializes screen/keyboard/mouse drivers and runs the event loop.
 * Renders directly to the framebuffer via QPainter.
 */
#include "include/qapplication_qt.h"
#include "include/qscreenhillsonfb_qws.h"
#include "include/qkbdhillson_qws.h"
#include "include/qmousehillson_qws.h"
#include "include/qrect_qt.h"
#include "include/qcolor_qt.h"
#include "qwidget.h"
#include "qpainter.h"

#define SYS_GUI_FB_INFO   70
#define SYS_GUI_INPUT_READ 72
#define SYS_YIELD          3

// ============================================================
// Syscall wrappers
// ============================================================

int gui_get_fb_info(FbInfo *fb) {
    int r = -1;
    __asm__ volatile("int $0x80"
        : "=a"(r)
        : "a"(SYS_GUI_FB_INFO), "b"(fb)
        : "memory", "cc");
    return r;
}

static void sys_yield() {
    __asm__ volatile("int $0x80" :: "a"(SYS_YIELD) : "memory", "cc");
}

// ============================================================
// QApplication
// ============================================================

QApplication *QApplication::s_instance = 0;

QApplication::QApplication()
    : m_screen(0), m_kbd(0), m_mouse(0), m_mainWidget(0), m_quit(false)
{
    s_instance = this;

    // 1. Create and connect screen driver
    m_screen = new QHillsonFbScreen(0);
    if (!m_screen->connect(0)) {
        // Failed — the desktop test can still work via direct fb writes
    }
    m_screen->initDevice();
    qt_screen = m_screen;

    // 2. Create keyboard handler
    m_kbd = new QHillsonKeyboardHandler();
    qt_keyboard_handler = m_kbd;

    // 3. Create mouse handler
    m_mouse = new QHillsonMouseHandler();
    qt_mouse_handler = m_mouse;
}

void QApplication::setMainWidget(QWidget *w)
{
    m_mainWidget = w;
}

int QApplication::exec()
{
    if (!m_mainWidget || !m_screen) return -1;

    int sw = m_screen->width();
    int sh = m_screen->height();
    unsigned int *fb = (unsigned int*)m_screen->base();

    // Create QPainter for framebuffer rendering
    QPainter painter(fb, sw, sh, m_screen->linestep());

    // Set main widget to fill screen
    m_mainWidget->setGeometry(0, 0, sw, sh);
    m_mainWidget->show();

    // Initial render
    m_mainWidget->render(&painter);

    // Event loop
    while (!m_quit) {
        sys_yield();

        bool needRender = false;

        // Poll keyboard
        if (m_kbd) {
            while (m_kbd->poll()) {
                int keycode = m_kbd->lastKeyCode();

                // ESC = quit
                if (keycode == Qt::Key_Escape) {
                    m_quit = true;
                    break;
                }

                // Forward to focused widget
                needRender = true;
            }
        }

        // Poll mouse
        if (m_mouse && m_mouse->poll(sw, sh)) {
            needRender = true;
        }

        // Re-render if needed
        if (needRender) {
            m_mainWidget->render(&painter);
        }
    }

    m_screen->shutdownDevice();
    return 0;
}
